/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>

#include <cci_configuration>

#include <module_factory_registery.h>

#include <libqemu-cxx/target/aarch64.h>

#include <arm.h>
#include <device.h>
#include <ports/qemu-target-signal-socket.h>
#include <ports/target.h>

class cpu_arm_cortexR5 : public QemuCpuArm
{
public:
    cci::cci_param<bool> p_start_powered_off;

    cpu_arm_cortexR5(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : cpu_arm_cortexR5(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    cpu_arm_cortexR5(sc_core::sc_module_name name, QemuInstance& inst)
        : QemuCpuArm(name, inst, "cortex-r5-arm")
        , p_start_powered_off("start_powered_off", false,
                              "Start and reset the CPU "
                              "in powered-off state")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuCpuArm::before_end_of_elaboration();

        qemu::CpuArm cpu(m_dev);

        cpu.set_prop_bool("start-powered-off", p_start_powered_off);
        cpu.set_prop_bool("reset-hivecs", true);
    }
};

extern "C" void module_register();
