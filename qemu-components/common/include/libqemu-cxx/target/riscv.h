/*
 *  This file is part of libqemu-cxx
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <libqemu-cxx/libqemu-cxx.h>

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
