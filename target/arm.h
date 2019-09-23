/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  Clement Deschamps and Luc Michel
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

class CpuArm : public Cpu {
public:
    static constexpr const char * const TYPE = "arm-cpu";

    CpuArm() = default;
    CpuArm(const CpuArm &) = default;
    CpuArm(const Object &o) : Cpu(o) {}

    void set_cp15_cbar(uint64_t cbar);
    void add_nvic_link();
};

class CpuAarch64 : public CpuArm {
public:
    static constexpr const char * const TYPE = "arm-cpu";

    CpuAarch64() = default;
    CpuAarch64(const CpuAarch64 &) = default;
    CpuAarch64(const Object &o) : CpuArm(o) {}

    void set_aarch64_mode(bool aarch64_mode);
};

class ArmNvic : public Device {
public:
    static constexpr const char * const TYPE = "armv7m_nvic";

    ArmNvic() = default;
    ArmNvic(const ArmNvic &) = default;
    ArmNvic(const Object &o) : Device(o) {}

    void add_cpu_link();
};

}
