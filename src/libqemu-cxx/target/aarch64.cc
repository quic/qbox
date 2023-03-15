/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  GreenSocs
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

#include <libqemu/libqemu.h>

#include "libqemu-cxx/target/aarch64.h"
#include "../internals.h"

namespace qemu {

void CpuArm::set_cp15_cbar(uint64_t cbar) {
    m_int->exports().cpu_arm_set_cp15_cbar(m_obj, cbar);
}

void CpuArm::add_nvic_link() {
    m_int->exports().cpu_arm_add_nvic_link(m_obj);
}

uint64_t CpuArm::get_exclusive_addr() const {
    return m_int->exports().cpu_arm_get_exclusive_addr(m_obj);
}

uint64_t CpuArm::get_exclusive_val() const {
    return m_int->exports().cpu_arm_get_exclusive_val(m_obj);
}

void CpuArm::set_exclusive_val(uint64_t val) {
    m_int->exports().cpu_arm_set_exclusive_val(m_obj, val);
}

void CpuAarch64::set_aarch64_mode(bool aarch64_mode) {
    m_int->exports().cpu_aarch64_set_aarch64_mode(m_obj, aarch64_mode);
}

void ArmNvic::add_cpu_link() {
    m_int->exports().arm_nvic_add_cpu_link(m_obj);
}

} // namespace qemu
