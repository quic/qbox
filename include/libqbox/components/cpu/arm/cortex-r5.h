/*
 *  This file is part of libqbox
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

#include <string>

#include <cci_configuration>

#include <libqemu-cxx/target/aarch64.h>

#include "libqbox/components/cpu/cpu.h"
#include "libqbox/components/device.h"
#include "libqbox/ports/target-signal-socket.h"
#include "libqbox/ports/target.h"

class CpuArmCortexR5 : public QemuCpu {
public:
    static constexpr qemu::Target ARCH = qemu::Target::AARCH64;

public:
    cci::cci_param<bool> p_start_powered_off;

    CpuArmCortexR5(sc_core::sc_module_name name, QemuInstance& inst)
        : QemuCpu(name, inst, "cortex-r5-arm")
        , p_start_powered_off("start_powered_off", false, "Start and reset the CPU "
                                                          "in powered-off state")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuArm cpu(m_dev);

        cpu.set_prop_bool("start-powered-off", p_start_powered_off);
        cpu.set_prop_bool("reset-hivecs", true);

    }
};
