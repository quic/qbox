/*
 *  This file is part of libqemu-cxx
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

#include "../libqemu-cxx.h"

namespace qemu {
    class CpuRiscv64 : public Cpu {
    public:
        static constexpr const char *const TYPE = "rv64-riscv-cpu";

        CpuRiscv64() = default;
        CpuRiscv64(const CpuRiscv64 &) = default;
        CpuRiscv64(const Object &o) : Cpu(o) {}
    };

    class RiscvSifivePlic : public Device {
    public:
        static constexpr const char * const TYPE = "riscv.sifive.plic";

        RiscvSifivePlic() = default;
        RiscvSifivePlic(const RiscvSifivePlic &) = default;
        RiscvSifivePlic(const Object &o) : Device(o) {}

    };
}