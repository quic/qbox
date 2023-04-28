/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LIBQBOX_COMPONENTS_DISPLAY_H
#define _LIBQBOX_COMPONENTS_DISPLAY_H

#include <systemc>
#include <scp/report.h>

#include <greensocs/libgssync.h>

#include <libqemu/libqemu.h>
#include <libqbox/qemu-instance.h>
#include <libqbox/components/gpu/virtio-mmio-gpugl.h>
#include <libqbox-extra/components/pci/virtio-gpu-gl-pci.h>

#include <SDL.h>
#include <SDL_syswm.h>

/**
 * @class QemuDisplay
 *
 * @brief Qemu Display abstraction as a SystemC module
 *
 * @details This class abstracts a Qemu display as a SystemC module.
 */
class QemuDisplay : public sc_core::sc_module
{
private:
    static QemuInstance* inst;

    qemu::DclOps m_ops;
    gs::RunOnSysC m_on_sysc;
    bool m_realized = false;
    std::vector<qemu::SDL2Console> m_sdl2_consoles;
    qemu::DisplayGLCtxOps m_gl_ctx_ops;
    bool m_instantiated = false;

    QemuDisplay(const sc_core::sc_module_name& name, QemuDevice& gpu): sc_module(name) {
        QemuInstance* gpu_qemu_inst = &gpu.get_qemu_inst();
        if (inst && inst != gpu_qemu_inst) {
            SCP_FATAL(SCMOD) << "Can not create another QemuDisplay on a different QemuInstance";
        }
        QemuDisplay::inst = gpu_qemu_inst;
    }

public:
    bool is_realized() { return m_realized; }

    void instantiate() {
        if (m_instantiated) {
            return;
        }

        inst->get().enable_opengl();

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SCP_WARN(SCMOD) << "Skipping Display module: Failed to initialize SDL: "
                            << SDL_GetError();
            return;
        }

        // Prefer loading ANGLE GLES driver
        SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");

        m_instantiated = true;
    }

    static void gl_switch(DisplayChangeListener* dcl, DisplaySurface* new_surface) {
        qemu::LibQemu& lib = inst->get();
        QemuDisplay* display = reinterpret_cast<QemuDisplay*>(lib.dcl_new(dcl).get_user_data());

        // SDL2 GL switch should run on main kernel thread as it may resize the window. At
        // initialization time, it is scheduled to run on SystemC kernel thread without waiting for
        // its completion. This will prevent the deadlocking situation where SystemC is not running
        // the switch "on_sysc" as it is waiting for QEMU to "finish/yeld", but that does not happen
        // as QEMU is in turn waiting for its switch function to run on SystemC kernel thread.
        if (display->m_realized) {
            lib.unlock_iothread();
            display->m_on_sysc.run_on_sysc(
                [&lib, dcl, new_surface]() { lib.sdl2_gl_switch(dcl, new_surface); }, true);
            lib.lock_iothread();
        } else {
            display->m_on_sysc.run_on_sysc(
                [&lib, dcl, new_surface]() { lib.sdl2_gl_switch(dcl, new_surface); }, false);
        }
    }

    static void gl_update(DisplayChangeListener* dcl, int x, int y, int w, int h) {
        // Luckily for us the GL update interacts only with the Graphics API without touching the
        // window hence no need to run it on main thread.
        inst->get().sdl2_gl_update(dcl, x, y, w, h);
    }

    static void gl_refresh(DisplayChangeListener* dcl) {
        qemu::LibQemu& lib = inst->get();
        QemuDisplay* display = reinterpret_cast<QemuDisplay*>(lib.dcl_new(dcl).get_user_data());
        // SDL2 GL refresh should run on main kernel thread as it polls events
        if (display->m_realized) {
            lib.unlock_iothread();
            display->m_on_sysc.run_on_sysc([&lib, dcl]() { lib.sdl2_gl_refresh(dcl); }, true);
            lib.lock_iothread();
        } else {
            display->m_on_sysc.run_on_sysc([&lib, dcl]() { lib.sdl2_gl_refresh(dcl); }, false);
        }
    }

    static bool is_compatible_dcl(DisplayGLCtx* ctx, DisplayChangeListener* dcl) {
        QemuDisplay* display = reinterpret_cast<QemuDisplay*>(
            inst->get().dcl_new(dcl).get_user_data());
        return display->m_ops.is_used_by(dcl);
    }

    void realize() {
        using namespace std::placeholders;

        if (!m_instantiated || m_realized) {
            return;
        }

        qemu::LibQemu& lib = inst->get();

        // We do not call sdl2_display_init. Instead, we set our own display
        // callbacks which would run on the SystemC thread.

        std::vector<qemu::Console> consoles = lib.get_all_consoles();
        if (consoles.empty()) {
            SCP_FATAL(SCMOD) << "Failed to get any console. Please make sure graphics device is "
                                "realized before the display.";
        }

        qemu::DisplayOptions dpy_opts = lib.display_options_new();

        for (int i = 0; i < consoles.size(); ++i) {
            qemu::Console& cons = consoles[i];
            qemu::SDL2Console sdl2_console = lib.sdl2_console_new(cons, this);

            if (!cons.is_graphic() && cons.get_index() != 0) {
                sdl2_console.set_hidden(i != 0);
            }
            sdl2_console.set_idx(i);
            sdl2_console.set_opengl(true);
            sdl2_console.set_opts(dpy_opts);

            m_ops = lib.dcl_ops_new();
            m_ops.set_name("sc");
            m_ops.set_gfx_switch(&gl_switch);
            m_ops.set_gfx_update(&gl_update);
            m_ops.set_refresh(&gl_refresh);
            sdl2_console.set_dcl_ops(m_ops);

            m_gl_ctx_ops = lib.display_gl_ctx_ops_new(&is_compatible_dcl);
            sdl2_console.set_dgc_ops(m_gl_ctx_ops);

            cons.set_display_gl_ctx(sdl2_console.get_dgc());
            sdl2_console.register_dcl();

#ifdef __APPLE__
            SDL_SysWMinfo info;
            SDL_GetWindowWMInfo(sdl2_console.get_real_window(), &info);
            cons.set_window_id((uintptr_t)info.info.cocoa.window);
#endif

            m_sdl2_consoles.push_back(std::move(sdl2_console));
        }

        m_realized = true;
    }

    /**
     * @brief Construct a QEMUDisplay
     *
     * @param[in] name SystemC module name
     * @param[in] gpu GPU module associated to this diplay.
     */
    QemuDisplay(const sc_core::sc_module_name& name, QemuVirtioGpuGlPci& gpu)
        : QemuDisplay(name, reinterpret_cast<QemuDevice&>(gpu)) {}

    /**
     * @brief Construct a QEMUDisplay
     *
     * @param[in] name SystemC module name
     * @param[in] gpu GPU module associated to this display
     */
    QemuDisplay(const sc_core::sc_module_name& name, QemuVirtioMMIOGpuGl& gpu)
        : QemuDisplay(name, reinterpret_cast<QemuDevice&>(gpu)) {}

    virtual ~QemuDisplay() {
        // TODO: need a better strategy for teardown
        m_ops.set_name("deleted");
        m_ops.set_gfx_switch(nullptr);
        m_ops.set_gfx_update(nullptr);
        m_ops.set_refresh(nullptr);
    }

    virtual void before_end_of_elaboration() override { instantiate(); }

    virtual void end_of_elaboration() override { realize(); }

    QemuInstance& get_qemu_inst() { return *inst; }

    bool is_instantiated() const { return m_instantiated; }

    const std::vector<qemu::SDL2Console>& get_sdl2_consoles() const { return m_sdl2_consoles; }
};

QemuInstance* QemuDisplay::inst = nullptr;

#endif // _LIBQBOX_COMPONENTS_DISPLAY_H
