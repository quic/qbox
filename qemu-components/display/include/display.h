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

class display : public sc_core::sc_module
{
private:
#ifdef __APPLE__
    MainThreadQemuDisplay m_main_display;
#endif

    display (const sc_core::sc_module_name& name, QemuDevice& gpu);

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
