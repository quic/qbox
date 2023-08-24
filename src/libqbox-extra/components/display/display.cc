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

#include <libqbox-extra/components/display/display.h>

MainThreadQemuDisplay::~MainThreadQemuDisplay()
{
    // TODO: need a better strategy for teardown
    m_ops.set_name("deleted");
    m_ops.set_gfx_switch(nullptr);
    m_ops.set_gfx_update(nullptr);
    m_ops.set_refresh(nullptr);
    inst->get().sdl2_cleanup();
}

MainThreadQemuDisplay::MainThreadQemuDisplay(QemuDevice* gpu)
{
    if (!gpu) {
        SCP_FATAL() << "Ivalid GPU";
    }
    QemuInstance* gpu_qemu_inst = &gpu->get_qemu_inst();
    if (inst && inst != gpu_qemu_inst) {
        SCP_FATAL() << "Can not create another MainThreadQemuDisplay on a different QemuInstance";
    }
    MainThreadQemuDisplay::inst = gpu_qemu_inst;
}

MainThreadQemuDisplay::MainThreadQemuDisplay(sc_core::sc_object* o): MainThreadQemuDisplay(selectGpu(o)) {}

MainThreadQemuDisplay::MainThreadQemuDisplay(QemuVirtioGpu& gpu)
    : MainThreadQemuDisplay(reinterpret_cast<QemuDevice*>(&gpu))
{
}

MainThreadQemuDisplay::MainThreadQemuDisplay(QemuVirtioMMIOGpuGl& gpu)
    : MainThreadQemuDisplay(reinterpret_cast<QemuDevice*>(&gpu))
{
}

QemuDevice* MainThreadQemuDisplay::selectGpu(sc_core::sc_object* o)
{
    if (dynamic_cast<QemuVirtioMMIOGpuGl*>(o) != nullptr) {
        return dynamic_cast<QemuVirtioMMIOGpuGl*>(o);
    } else if (dynamic_cast<QemuVirtioGpu*>(o) != nullptr) {
        return dynamic_cast<QemuVirtioGpu*>(o);
    } else {
        SCP_FATAL() << "The type of the object 'o' is not a QemuVirtioMMIOGpuGl or QemuVirtioGpu";
        return nullptr;
    }
}

void MainThreadQemuDisplay::instantiate()
{
    if (m_instantiated) {
        return;
    }

    inst->get().enable_opengl();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SCP_WARN() << "Skipping Display module: Failed to initialize SDL: " << SDL_GetError();
        return;
    }

    // Prefer loading ANGLE GLES driver
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");

    m_instantiated = true;
}

void MainThreadQemuDisplay::gl_switch(DisplayChangeListener* dcl, DisplaySurface* new_surface)
{
    qemu::LibQemu& lib = inst->get();
    MainThreadQemuDisplay* display = reinterpret_cast<MainThreadQemuDisplay*>(lib.dcl_new(dcl).get_user_data());

    if (display->m_simulation_started) {
        lib.unlock_iothread();
    }

    // SDL2 GL switch should run on main kernel thread as it may resize the window. At
    // initialization time, it is scheduled to run on SystemC kernel thread without waiting for
    // its completion. This will prevent the deadlocking situation where SystemC is not running
    // the switch "on_sysc" as it is waiting for QEMU to "finish/yeld", but that does not happen
    // as QEMU is in turn waiting for its switch function to run on SystemC kernel thread.
    display->m_on_sysc.run_on_sysc(
            [&lib, dcl, new_surface]() { lib.sdl2_gl_switch(dcl, new_surface); },
            display->m_simulation_started);
    if (display->m_simulation_started) {
        lib.lock_iothread();
    }
}

void MainThreadQemuDisplay::gl_update(DisplayChangeListener* dcl, int x, int y, int w, int h)
{
    // Luckily for us the GL update interacts only with the Graphics API without touching the
    // window hence no need to run it on main thread.
    inst->get().sdl2_gl_update(dcl, x, y, w, h);
}

void MainThreadQemuDisplay::gl_refresh(DisplayChangeListener* dcl)
{
    qemu::LibQemu& lib = inst->get();
    MainThreadQemuDisplay* display = reinterpret_cast<MainThreadQemuDisplay*>(lib.dcl_new(dcl).get_user_data());
    if (display->m_simulation_started) {
        lib.unlock_iothread();
    }

    // SDL2 GL refresh should run on main kernel thread as it polls events
    display->m_on_sysc.run_on_sysc([&lib, dcl]() { lib.sdl2_gl_refresh(dcl); },
                                       display->m_simulation_started);

    if (display->m_simulation_started) {
        lib.lock_iothread();
    }
}

bool MainThreadQemuDisplay::is_compatible_dcl(DisplayGLCtx* ctx, DisplayChangeListener* dcl)
{
    MainThreadQemuDisplay* display = reinterpret_cast<MainThreadQemuDisplay*>(inst->get().dcl_new(dcl).get_user_data());
    return display->m_ops.is_used_by(dcl);
}

void MainThreadQemuDisplay::realize()
{
    using namespace std::placeholders;

    if (!m_instantiated || m_realized) {
        return;
    }

    qemu::LibQemu& lib = inst->get();

    // We do not call sdl2_display_init. Instead, we set our own display
    // callbacks which would run on the SystemC thread.

    std::vector<qemu::Console> consoles = lib.get_all_consoles();
    if (consoles.empty()) {
        SCP_FATAL() << "Failed to get any console. Please make sure graphics device is "
                       "realized before the display.";
    }

    qemu::DisplayOptions dpy_opts = lib.display_options_new();

    m_sdl2_consoles = lib.sdl2_create_consoles(consoles.size());

    for (int i = 0; i < consoles.size(); ++i) {
        qemu::Console& cons = consoles[i];
        qemu::SDL2Console& sdl2_console = m_sdl2_consoles[i];
        sdl2_console.init(cons, this);

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
    }

    m_realized = true;
}

QemuDisplay::QemuDisplay(const sc_core::sc_module_name& name, sc_core::sc_object* o)
    : sc_module(name)
#ifdef __APPLE__
    // On MacOS use libqbox's QemuDisplay SystemC module.
    ,m_main_display(o)
#endif
{
#ifndef __APPLE__
    // Use QEMU's integrated display only if we are NOT on MacOS.
    // On MacOS use libqbox's QemuDisplay SystemC module.
    QemuDevice* gpu = (dynamic_cast<QemuDevice*>(o));
    gpu->get_qemu_inst().set_display_arg("sdl,gl=on");
#endif
}

QemuDisplay::QemuDisplay(const sc_core::sc_module_name& name, QemuDevice& gpu)
    : QemuDisplay(name, reinterpret_cast<sc_core::sc_object*>(&gpu))
{
}

QemuDisplay::QemuDisplay(const sc_core::sc_module_name& name, QemuVirtioGpu& gpu)
    : QemuDisplay(name, reinterpret_cast<sc_core::sc_object*>(&gpu))
{
}

QemuDisplay::QemuDisplay(const sc_core::sc_module_name& name, QemuVirtioMMIOGpuGl& gpu)
    : QemuDisplay(name, reinterpret_cast<sc_core::sc_object*>(&gpu))
{
}

void QemuDisplay::before_end_of_elaboration()
{
#ifdef __APPLE__
    m_main_display.instantiate();
#endif
}

void QemuDisplay::end_of_elaboration()
{
#ifdef __APPLE__
    m_main_display.realize();
#endif
}

QemuInstance* QemuDisplay::get_qemu_inst()
{
#ifdef __APPLE__
    return &m_main_display.get_qemu_inst();
#else
    return nullptr;
#endif
}

bool QemuDisplay::is_instantiated() const
{
#ifdef __APPLE__
    return m_main_display.is_instantiated();
#else
    return true;
#endif
}

bool QemuDisplay::is_realized() const
{
#ifdef __APPLE__
    return m_main_display.is_realized();
#else
    return true;
#endif
}

const std::vector<qemu::SDL2Console>* QemuDisplay::get_sdl2_consoles() const
{
#ifdef __APPLE__
    return &m_main_display.get_sdl2_consoles();
#else
    return nullptr;
#endif
}
