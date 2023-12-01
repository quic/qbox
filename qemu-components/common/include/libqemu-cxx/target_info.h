/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2021 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

namespace qemu {

enum Target {
    AARCH64,
    RISCV64,
    RISCV32,
    MICROBLAZE,
    MICROBLAZEEL,
    HEXAGON,
};

const char* get_target_name(Target t);
const char* get_target_lib(Target t);

} // namespace qemu
