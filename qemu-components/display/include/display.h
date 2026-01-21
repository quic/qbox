/*
 *  This file is part of libqbox
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_DISPLAY_H
#define _LIBQBOX_COMPONENTS_DISPLAY_H

#include <systemc>
#include <scp/report.h>
#include <string_view>

#include <libgssync.h>
#include <module_factory_registery.h>

#include <libqemu/libqemu.h>
#include <qemu-instance.h>
#include <virtio/virtio-mmio-gpugl.h>
#include <virtio_gpu.h>

/**
 * @class MainThreadQemuDisplay
 *
 * @brief Qemu Display abstraction as a SystemC module
 *
 * @details This class abstracts a Qemu display as a SystemC module.
 */
class MainThreadQemuDisplay
{
private:
    static QemuInstance* inst;

    qemu::DclOps m_ops;
    gs::runonsysc m_on_sysc;
    bool m_realized = false;
    std::vector<qemu::SDL2Console> m_sdl2_consoles;
    qemu::DisplayGLCtxOps m_gl_ctx_ops;
    bool m_instantiated = false;
    bool m_simulation_started = false;

    QemuDevice* selectGpu(sc_core::sc_object* o);

public:
    bool is_realized() const { return m_realized; }

    void instantiate();

    static void gl_switch(DisplayChangeListener* dcl, DisplaySurface* new_surface);

    static void gl_update(DisplayChangeListener* dcl, int x, int y, int w, int h);

    static void gl_refresh(DisplayChangeListener* dcl);

    static void window_create(DisplayChangeListener* dcl);
    static void window_destroy(DisplayChangeListener* dcl);
    static void window_resize(DisplayChangeListener* dcl);
    static void poll_events(DisplayChangeListener* dcl);

    static bool is_compatible_dcl(DisplayGLCtx* ctx, DisplayChangeListener* dcl);

    void realize();

    MainThreadQemuDisplay(QemuDevice* gpu);
    /**
     * @brief Construct a MainThreadQemuDisplay
     *
     * @param[in] gpu GPU module associated to this diplay.
     */
    MainThreadQemuDisplay(sc_core::sc_object* gpu);

    /**
     * @brief Construct a MainThreadQemuDisplay
     *
     * @param[in] name SystemC module name
     * @param[in] gpu GPU module associated to this diplay.
     */
    MainThreadQemuDisplay(QemuVirtioGpu& gpu);

    /**
     * @brief Construct a MainThreadQemuDisplay
     *
     * @param[in] name SystemC module name
     * @param[in] gpu GPU module associated to this display
     */
    MainThreadQemuDisplay(QemuVirtioMMIOGpuGl& gpu);

    ~MainThreadQemuDisplay();

    void before_end_of_elaboration() { instantiate(); }

    void end_of_elaboration() { realize(); }

    void start_of_simulation(void) { m_simulation_started = true; }

    QemuInstance& get_qemu_inst() { return *inst; }

    bool is_instantiated() const { return m_instantiated; }

    const std::vector<qemu::SDL2Console>& get_sdl2_consoles() const { return m_sdl2_consoles; }
};

QemuInstance* MainThreadQemuDisplay::inst = nullptr;

/**
 * @class VncConfiguration
 *
 * @brief VNC Configuration as a systemC module
 *
 * @details This class manages the configuration of a VNC Display.
 *
 * It performns the followings tasks
 *    * Spoof the display device id (gpu-virtio or vga-virtio)
 *    * and display number provided by the users
 *    * Pass to libqemu the remain optitional configuration items.
 */

class VncConfiguration : public sc_core::sc_module
{
    SCP_LOGGER(());

public:
    cci::cci_param<bool> p_enabled;

protected:
    std::string _qemu_opts;
    ssize_t _display_index;
    std::string _display_device_id;
    std::vector<std::string> spoofed_options = { ":", "head=", "display=" };

public:
    /**
     * @brief Construct a new VNC VncConfiguration
     *
     * @param[in] name SystemC module name
     * @param[in] display_index The index of the display that must be spoofed
     * @param[in] display_device_id The index of the display device id that must be spoofed
     */

    VncConfiguration(const sc_core::sc_module_name& n, ssize_t display_index, const std::string& display_device_id)
        : sc_module(n)
        , _display_index(display_index)
        , _display_device_id(display_device_id)
        , p_enabled(std::string("enable"), false, "Default state")
    {
        cci::cci_param<std::string> params_input(std::string("params"), "", "Qemu VNC configuration");

        if (p_enabled.get_value()) {
            SCP_INFO(())
            ("VNC Configuration on display: {}, device id: {} and user param: {}", display_index, display_device_id,
             params_input.get_value());
            build_qemu_vnc_arguments(params_input);
            SCP_INFO(())("Qemu parameter: {}", _qemu_opts);
        }
    }

    bool is_enabled() const { return p_enabled.get_value(); }

    std::string_view get_qemu_opts() const { return std::string_view(_qemu_opts); }

private:
    void build_qemu_vnc_arguments(const std::string& user_options)
    {
        std::stringstream user_options_stream(user_options);
        std::vector<std::string> option_tokens;

        /* Filter out spoofed arguments */
        for (std::string option; std::getline(user_options_stream, option, ',');) {
            if (!option.empty() && !spoofed_opt(option)) {
                option_tokens.push_back(option);
            }
        }

        /* Add display index */
        _qemu_opts.append(get_display_opt(',')).append(get_head_opt(','));

        /* Append non-spoofed options */
        for (const auto& option : option_tokens) {
            _qemu_opts.append(option + ',');
        }

        // `Display device id` is the last in the options list
        // and doesn't need to end with a comma
        _qemu_opts.append(get_display_device_opt());
    }

    bool spoofed_opt(const std::string_view& option)
    {
        for (const std::string& spoofed_opt : spoofed_options) {
            if (option.find(spoofed_opt) == 0) {
                SCP_WARN(())("Option {} provided by the user will be ignored.", spoofed_opt);
                return true;
            }
        }
        return false;
    };

    std::string get_head_opt(char append_suffix = '\0')
    {
        std::string head_opt = "head=" + std::to_string(_display_index);
        if (append_suffix != '\0') {
            head_opt += append_suffix;
        }
        return head_opt;
    }

    std::string get_display_opt(char append_suffix = '\0')
    {
        std::string display_opt = ":" + std::to_string(_display_index);
        if (append_suffix != '\0') {
            display_opt += append_suffix;
        };
        return display_opt;
    }

    std::string get_display_device_opt(char append_suffix = '\0')
    {
        std::string display_device = "display=" + _display_device_id;
        if (append_suffix != '\0') {
            display_device += append_suffix;
        }
        return display_device;
    }
};

class display : public sc_core::sc_module
{
    SCP_LOGGER();

protected:
    sc_core::sc_vector<VncConfiguration> vnc;

private:
#ifdef __APPLE__
    MainThreadQemuDisplay m_main_display;
#endif

    display(const sc_core::sc_module_name& name, QemuDevice& gpu);

    bool is_vnc_enabled()
    {
        for (const auto& vnc_conf : vnc) {
            if (vnc_conf.is_enabled()) {
                return true;
            }
        }
        return false;
    }

    void fill_vnc_options(std::vector<std::string>& vnc_options)
    {
        for (const auto& vnc_conf : vnc) {
            if (vnc_conf.is_enabled()) {
                vnc_options.push_back(std::string(vnc_conf.get_qemu_opts()));
            }
        }
    }

public:
    /**
     * @brief Construct a display
     *
     * @param[in] name SystemC module name
     * @param[in] gpu GPU module associated to this diplay.
     */
    display(const sc_core::sc_module_name& name, sc_core::sc_object* gpu);

    /**
     * @brief Construct a display
     *
     * @param[in] name SystemC module name
     * @param[in] gpu GPU module associated to this diplay.
     */
    display(const sc_core::sc_module_name& name, QemuVirtioGpu& gpu);

    /**
     * @brief Construct a display
     *
     * @param[in] name SystemC module name
     * @param[in] gpu GPU module associated to this display
     */
    display(const sc_core::sc_module_name& name, QemuVirtioMMIOGpuGl& gpu);

    void before_end_of_elaboration() override;

    void end_of_elaboration() override;

    void start_of_simulation() override;

    QemuInstance* get_qemu_inst();

    bool is_instantiated() const;
    bool is_realized() const;

    const std::vector<qemu::SDL2Console>* get_sdl2_consoles() const;
};

extern "C" void module_register();

#endif // _LIBQBOX_COMPONENTS_DISPLAY_H
