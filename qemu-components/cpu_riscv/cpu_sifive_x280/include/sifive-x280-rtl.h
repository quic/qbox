/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <functional>

#include <libqemu-cxx/target/riscv.h>

#include <module_factory_registery.h>

#include <libgssync.h>

#include <cpu.h>

class cpu_sifive_X280 : public QemuCpu
{
protected:
    uint64_t m_hartid;
    gs::async_event m_irq_ev;

    void mip_update_cb(uint32_t value)
    {
        if (value) {
            m_irq_ev.notify();
        }
    }

public:
    cpu_sifive_X280(const sc_core::sc_module_name& name, sc_core::sc_object* o, /*const char* model*/std::string model, uint64_t hartid)
        : cpu_sifive_X280(name, *(dynamic_cast<QemuInstance*>(o)), model, hartid)
    {
    }
    cpu_sifive_X280(const sc_core::sc_module_name& name, QemuInstance& inst, /*const char* model*/std::string model, uint64_t hartid)
        : QemuCpu(name, inst, /*std::string(model)*/ model + "-riscv")
        , m_hartid(hartid)
        /*
         * We have no choice but to attach-suspend here. This is fixable but
         * non-trivial. It means that the SystemC kernel will never starve...
         */
        , m_irq_ev(true)
    {
        m_external_ev |= m_irq_ev;
    }

    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuRiscv64SiFiveX280 cpu(get_qemu_dev());
        cpu.set_prop_int("hartid", m_hartid);

        cpu.set_mip_update_callback(std::bind(&cpu_sifive_X280::mip_update_cb, this, std::placeholders::_1));
    }

    // properties to be set from main.cc
};

extern "C" void module_register();
