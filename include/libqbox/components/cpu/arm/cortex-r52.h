/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>

#include <cci_configuration>

#include <libqemu-cxx/target/aarch64.h>

#include "libqbox/components/cpu/cpu.h"
#include "libqbox/components/device.h"
#include "libqbox/ports/target-signal-socket.h"
#include "libqbox/ports/target.h"

class CpuArmCortexR52 : public QemuCpu
{
public:
    static constexpr qemu::Target ARCH = qemu::Target::AARCH64;

public:
    cci::cci_param<bool> p_start_powered_off;

    CpuArmCortexR52(sc_core::sc_module_name name, QemuInstance& inst)
        : QemuCpu(name, inst, "cortex-r52-arm")
        , p_start_powered_off("start_powered_off", false,
                              "Start and reset the CPU "
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
