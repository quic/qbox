/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2020 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "libqemu-cxx/libqemu-cxx.h"

namespace qemu {

class CpuMicroblaze : public Cpu
{
public:
    static constexpr const char* const TYPE = "microblaze-cpu";

    CpuMicroblaze() = default;
    CpuMicroblaze(const CpuMicroblaze&) = default;
    CpuMicroblaze(const Object& o): Cpu(o) {}
};

} // namespace qemu
