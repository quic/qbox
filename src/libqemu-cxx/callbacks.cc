/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2021 GreenSocs
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

#include "libqemu-cxx/libqemu-cxx.h"
#include "internals.h"

namespace qemu {

static void generic_cpu_end_of_loop_cb(QemuObject* cpu, void* opaque)
{
    LibQemuInternals* internals = reinterpret_cast<LibQemuInternals*>(opaque);

    internals->get_cpu_end_of_loop_cb().call(cpu);
}

static void generic_cpu_kick_cb(QemuObject* cpu, void* opaque)
{
    LibQemuInternals* internals = reinterpret_cast<LibQemuInternals*>(opaque);

    internals->get_cpu_kick_cb().call(cpu);
}

static void generic_cpu_riscv_mip_update_cb(QemuObject* cpu, uint32_t value, void* opaque)
{
    LibQemuInternals* internals = reinterpret_cast<LibQemuInternals*>(opaque);

    internals->get_cpu_riscv_mip_update_cb().call(cpu, value);
}

void LibQemu::init_callbacks()
{
    m_int->exports().set_cpu_end_of_loop_cb(generic_cpu_end_of_loop_cb, m_int.get());

    m_int->exports().set_cpu_kick_cb(generic_cpu_kick_cb, m_int.get());

    if (m_int->exports().cpu_riscv_register_mip_update_callback != nullptr) {
        m_int->exports().cpu_riscv_register_mip_update_callback(generic_cpu_riscv_mip_update_cb, m_int.get());
    }
}

} // namespace qemu
