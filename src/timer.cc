/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  Clement Deschamps and Luc Michel
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

namespace qemu {

Timer::Timer(LibQemuExports &exports)
    : m_exports(exports)
{
}

Timer::~Timer()
{
    del();

    if (m_timer != nullptr) {
        m_exports.timer_free(m_timer);
    }
}

static void timer_generic_callback(void *opaque)
{
    Timer::TimerCallbackFn *cb =
        reinterpret_cast<Timer::TimerCallbackFn*>(opaque);

    (*cb)();
}

void Timer::set_callback(TimerCallbackFn cb)
{
    m_cb = cb;
    m_timer = m_exports.timer_new_virtual_ns(timer_generic_callback,
                                             reinterpret_cast<void*>(&m_cb));
}

void Timer::mod(int64_t deadline)
{
    if (m_timer != nullptr) {
        m_exports.timer_mod_ns(m_timer, deadline);
    }
}

void Timer::del()
{
    if (m_timer != nullptr) {
        m_exports.timer_del(m_timer);
    }
}

}
