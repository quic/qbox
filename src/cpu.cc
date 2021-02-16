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

void Cpu::loop()
{
    m_exports->cpu_loop(m_obj);
}

bool Cpu::loop_is_busy()
{
    return m_exports->cpu_loop_is_busy(m_obj);
}

bool Cpu::can_run()
{
    return m_exports->cpu_can_run(m_obj);
}

void Cpu::halt(bool halted)
{
    m_exports->cpu_halt(m_obj, halted);
}

void Cpu::reset()
{
    m_exports->cpu_reset(m_obj);
}

void Cpu::register_thread()
{
    m_exports->cpu_register_thread(m_obj);
}

Cpu Cpu::set_as_current()
{
    Cpu ret(Object(m_exports->current_cpu_get(), *m_exports));

    m_exports->current_cpu_set(m_obj);

    return ret;
}

void Cpu::kick()
{
    m_exports->cpu_kick(m_obj);
}

void Cpu::request_exit()
{
    m_exports->cpu_request_exit(m_obj);
}

void Cpu::async_safe_run(AsyncJobFn job, void *arg)
{
    m_exports->async_safe_run_on_cpu(m_obj, job, arg);
}

};

