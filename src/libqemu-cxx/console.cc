/*
 *  This file is part of libqemu-cxx
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

#include <libqemu/libqemu.h>

#include "libqemu-cxx/libqemu-cxx.h"
#include "internals.h"

namespace qemu {

DisplayOptions::DisplayOptions(::DisplayOptions* opts, std::shared_ptr<LibQemuInternals>& internals)
    : m_opts(opts), m_int(internals) {
}

Console::Console(QemuConsole* cons, std::shared_ptr<LibQemuInternals>& internals)
    : m_cons(cons), m_int(internals) {
}

int Console::get_index() const {
    return m_int->exports().console_get_index(m_cons);
}

bool Console::is_graphic() const {
    return m_int->exports().console_is_graphic(m_cons);
}

void Console::set_display_gl_ctx(DisplayGLCtx* gl_ctx) {
    m_int->exports().console_set_display_gl_ctx(m_cons, gl_ctx);
}

void Console::set_window_id(int id) {
    m_int->exports().console_set_window_id(m_cons, id);
}

SDL2Console::SDL2Console(struct sdl2_console* cons, std::shared_ptr<LibQemuInternals>& internals)
    : m_cons(cons), m_int(internals) {
}

void SDL2Console::set_hidden(bool hidden) {
    m_int->exports().sdl2_console_set_hidden(m_cons, hidden);
}

void SDL2Console::set_idx(int idx) {
    m_int->exports().sdl2_console_set_idx(m_cons, idx);
}

void SDL2Console::set_opts(DisplayOptions& opts) {
    m_int->exports().sdl2_console_set_opts(m_cons, opts.m_opts);
}

void SDL2Console::set_opengl(bool opengl) {
    m_int->exports().sdl2_console_set_opengl(m_cons, opengl);
}

void SDL2Console::set_dcl_ops(DclOps& dcl_ops) {
    m_int->exports().sdl2_console_set_dcl_ops(m_cons, dcl_ops.m_ops);
}

void SDL2Console::set_dgc_ops(DisplayGLCtxOps& dgc_ops) {
    m_int->exports().sdl2_console_set_dgc_ops(m_cons, dgc_ops.m_ops);
}

SDL_Window* SDL2Console::get_real_window() const {
    return m_int->exports().sdl2_console_get_real_window(m_cons);
}

DisplayChangeListener* SDL2Console::get_dcl() const {
    return m_int->exports().sdl2_console_get_dcl(m_cons);
}

DisplayGLCtx* SDL2Console::get_dgc() const {
    return m_int->exports().sdl2_console_get_dgc(m_cons);
}

void SDL2Console::register_dcl() const {
    m_int->exports().dcl_register(get_dcl());
}

DisplayGLCtxOps::DisplayGLCtxOps(::DisplayGLCtxOps* ops,
                                 std::shared_ptr<LibQemuInternals>& internals)
    : m_ops(ops), m_int(internals) {
}

Dcl::Dcl(DisplayChangeListener* dcl, std::shared_ptr<LibQemuInternals>& internals)
    : m_dcl(dcl), m_int(internals) {
}

void* Dcl::get_user_data() {
    return m_int->exports().dcl_get_user_data(m_dcl);
}

DclOps::DclOps(DisplayChangeListenerOps* ops, std::shared_ptr<LibQemuInternals>& internals)
    : m_ops(ops), m_int(internals) {
}

void DclOps::set_name(const char* name) {
    m_int->exports().dcl_ops_set_name(m_ops, name);
}

bool DclOps::is_used_by(DisplayChangeListener* dcl) const {
    return m_int->exports().dcl_get_ops(dcl) == m_ops;
}

void DclOps::set_gfx_update(LibQemuGfxUpdateFn gfx_update_fn) {
    m_int->exports().dcl_ops_set_gfx_update(m_ops, gfx_update_fn);
}

void DclOps::set_gfx_switch(LibQemuGfxSwitchFn gfx_switch_fn) {
    m_int->exports().dcl_ops_set_gfx_switch(m_ops, gfx_switch_fn);
}

void DclOps::set_refresh(LibQemuRefreshFn refresh_fn) {
    m_int->exports().dcl_ops_set_refresh(m_ops, refresh_fn);
}

} // namespace qemu
