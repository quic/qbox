/*
 *  This file is part of libqemu-cxx
 *  Copyright (c) 2021 Greensocs
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

#include "libqemu-cxx/libqemu-cxx.h"

namespace qemu {
class CpuRiscv : public Cpu
{
public:
    static constexpr const char* const TYPE = "riscv-cpu";

    using MipUpdateCallbackFn = std::function<void(uint32_t)>;

    CpuRiscv() = default;
    CpuRiscv(const CpuRiscv&) = default;
    CpuRiscv(const Object& o): Cpu(o) {}

    void set_mip_update_callback(MipUpdateCallbackFn cb);
};

class CpuRiscv32 : public CpuRiscv
{
public:
    CpuRiscv32() = default;
    CpuRiscv32(const CpuRiscv32&) = default;
    CpuRiscv32(const Object& o): CpuRiscv(o) {}
};

class CpuRiscv64 : public CpuRiscv
{
public:
    CpuRiscv64() = default;
    CpuRiscv64(const CpuRiscv64&) = default;
    CpuRiscv64(const Object& o): CpuRiscv(o) {}
};

class CpuRiscv64SiFiveX280 : public CpuRiscv
{
public:
    CpuRiscv64SiFiveX280() = default;
    CpuRiscv64SiFiveX280(const CpuRiscv64SiFiveX280&) = default;
    CpuRiscv64SiFiveX280(const Object& o): CpuRiscv(o) {}
};
} // namespace qemu
