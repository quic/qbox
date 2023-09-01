/*
 *  This file is part of libqbox
 * Copyright(c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>

#include <libqemu-cxx/target/aarch64.h>
#include <greensocs/gsutils/module_factory_registery.h>

#include "libqbox/components/irq-ctrl/armv7m-nvic.h"
#include "libqbox/components/cpu/cpu.h"

class CpuArmCortexM55 : public QemuCpu
{
public:
    static constexpr qemu::Target ARCH = qemu::Target::AARCH64;

    cci::cci_param<bool> p_start_powered_off;
    QemuNvicArmv7m m_nvic;
    cci::cci_param<uint64_t> p_init_nsvtor;
    CpuArmCortexM55(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : CpuArmCortexM55(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    CpuArmCortexM55(sc_core::sc_module_name name, QemuInstance& inst)
        : QemuCpu(name, inst, "cortex-m55-arm")
        , m_nvic("nvic", inst)
        , p_start_powered_off("start_powered_off", false,
                              "Start and reset the CPU "
                              "in powered-off state")
        , p_init_nsvtor("init_nsvtor", 0ull, "Reset vector base address")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuArm cpu(m_dev);

        cpu.add_nvic_link();
        cpu.set_prop_bool("start-powered-off", p_start_powered_off);
        cpu.set_prop_int("init-nsvtor", p_init_nsvtor);

        /* ensure the nvic is also created */
        m_nvic.before_end_of_elaboration();

        /* setup cpu&nvic links so that we can realize both objects */
        qemu::Device nvic = m_nvic.get_qemu_dev();
        cpu.set_prop_link("nvic", nvic);
        nvic.set_prop_link("cpu", cpu);
    }
};

GSC_MODULE_REGISTER(CpuArmCortexM55, sc_core::sc_object*);
