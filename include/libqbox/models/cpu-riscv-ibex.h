/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Greensocs
 *
 *  Author: Damien Hedde
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

#include "libqbox/libqbox.h"

#include <libqemu-cxx/target/riscv.h>

#include "plic-ibex.h"

class CpuRiscvIbex : public QemuCpu {
public:
    QemuRiscvIbexPlic plic;

    CpuRiscvIbex(sc_core::sc_module_name name, uint32_t plic_num_irq = 80)
        : QemuCpu(name, "riscv32", "lowrisc-ibex-riscv")
        , plic("plic", this, plic_num_irq)
    {
    }
};
