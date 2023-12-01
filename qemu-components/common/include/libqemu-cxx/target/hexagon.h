/*
 *  This file is part of libqemu-cxx
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <libqemu-cxx/libqemu-cxx.h>

namespace qemu {
class CpuHexagon : public Cpu
{
public:
    static constexpr const char* const TYPE = "v67-hexagon-cpu";

    using MipUpdateCallbackFn = std::function<void(uint32_t)>;

    CpuHexagon() = default;
    CpuHexagon(const CpuHexagon&) = default;
    CpuHexagon(const Object& o): Cpu(o) {}
};
} // namespace qemu
