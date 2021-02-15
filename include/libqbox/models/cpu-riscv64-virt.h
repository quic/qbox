/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Lukas Jünger
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

#pragma once

#include "libqbox/libqbox.h"

#include <libqemu-cxx/target/riscv64.h>

class QemuCpuRiscv64Virt : public QemuCpu {
protected:
    uint64_t m_hartid;

public:
    QemuCpuRiscv64Virt(sc_core::sc_module_name name, uint64_t hartid)
        : QemuCpu(name, qemu::RISCV64, "rv64-riscv")
        , m_hartid(hartid)
    {}


    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuRiscv64 cpu(get_qemu_obj());
        cpu.set_prop_int("hartid", m_hartid);
    }
};
