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

namespace qemu {

const char* get_target_name(Target t)
{
    switch (t) {
    case AARCH64:
        return "AArch64";

    case RISCV64:
        return "RISC-V (64 bits)";

    case RISCV32:
        return "RISC-V (32 bits)";

    case MICROBLAZE:
        return "Microblaze";

    case MICROBLAZEEL:
        return "Microblaze (little-endian)";

    case HEXAGON:
        return "Hexagon";

    default:
        return "";
    }
}

#ifndef LIBQEMU_TARGET_aarch64_LIBRARY
#define LIBQEMU_TARGET_aarch64_LIBRARY nullptr
#endif

#ifndef LIBQEMU_TARGET_riscv64_LIBRARY
#define LIBQEMU_TARGET_riscv64_LIBRARY nullptr
#endif

#ifndef LIBQEMU_TARGET_riscv32_LIBRARY
#define LIBQEMU_TARGET_riscv32_LIBRARY nullptr
#endif

#ifndef LIBQEMU_TARGET_microblaze_LIBRARY
#define LIBQEMU_TARGET_microblaze_LIBRARY nullptr
#endif

#ifndef LIBQEMU_TARGET_microblazeel_LIBRARY
#define LIBQEMU_TARGET_microblazeel_LIBRARY nullptr
#endif

#ifndef LIBQEMU_TARGET_hexagon_LIBRARY
#define LIBQEMU_TARGET_hexagon_LIBRARY nullptr
#endif

const char* get_target_lib(Target t)
{
    switch (t) {
    case AARCH64:
        return LIBQEMU_TARGET_aarch64_LIBRARY;

    case RISCV64:
        return LIBQEMU_TARGET_riscv64_LIBRARY;

    case RISCV32:
        return LIBQEMU_TARGET_riscv32_LIBRARY;

    case MICROBLAZE:
        return LIBQEMU_TARGET_microblaze_LIBRARY;

    case MICROBLAZEEL:
        return LIBQEMU_TARGET_microblazeel_LIBRARY;

    case HEXAGON:
        return LIBQEMU_TARGET_hexagon_LIBRARY;

    default:
        return "";
    }
}

} // namespace qemu
