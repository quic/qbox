/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <vector>

#include <cci_configuration>

#include <libqemu-cxx/target/riscv.h>
#include <module_factory_registery.h>

#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <device.h>

class riscv_aclint_mtimer : public QemuDevice
{
public:
    cci::cci_param<uint32_t> p_hartid_base;
    cci::cci_param<unsigned int> p_num_harts;
    cci::cci_param<uint64_t> p_timecmp_base;
    cci::cci_param<uint64_t> p_time_base;
    cci::cci_param<uint64_t> p_aperture_size;
    cci::cci_param<uint32_t> p_timebase_freq;
    cci::cci_param<bool> p_provide_rdtime;

    QemuTargetSocket<> socket;
    sc_core::sc_vector<QemuInitiatorSignalSocket> timer_irq;

    riscv_aclint_mtimer(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : riscv_aclint_mtimer(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    riscv_aclint_mtimer(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "riscv.aclint.mtimer")
        , p_hartid_base("hartid_base", 0, "Base hart ID for this ACLINT MTimer")
        , p_num_harts("num_harts", 0, "Number of HARTS this CLINT is connected to")
        , p_timecmp_base("timecmp_base", 0, "Base address for the TIMECMP registers")
        , p_time_base("time_base", 0, "Base address for the TIME registers")
        , p_aperture_size("aperture_size", 0, "Size of the whole CLINT address space")
        , p_timebase_freq("timebase_freq", 10000000, "")
        , p_provide_rdtime("provide_rdtime", false,
                           "If true, provide the CPU with "
                           "a rdtime register")
        , socket("mem", inst)
        , timer_irq("timer_irq")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        // Initialize timer_irq vector with the correct size based on p_num_harts (if not already done)
        if (timer_irq.size() == 0) {
            timer_irq.init(p_num_harts, [](const char* n, size_t i) { return new QemuInitiatorSignalSocket(n); });
        }

        if (timer_irq.size() != p_num_harts) {
            SCP_WARN(SCMOD) << "Timer IRQ size mismatch: timer_irq.size()=" << timer_irq.size()
                           << " != p_num_harts=" << (unsigned int)p_num_harts;
        }

        m_dev.set_prop_int("hartid-base", p_hartid_base);
        m_dev.set_prop_int("num-harts", p_num_harts);
        m_dev.set_prop_int("timecmp-base", p_timecmp_base);
        m_dev.set_prop_int("time-base", p_time_base);
        m_dev.set_prop_int("aperture-size", p_aperture_size);
        m_dev.set_prop_int("timebase-freq", p_timebase_freq);
        m_dev.set_prop_bool("provide-rdtime", p_provide_rdtime);
    }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_dev());
        socket.init(qemu::SysBusDevice(m_dev), 0);

        // Initialize timer interrupt outputs for all harts (GPIO outputs)
        for (unsigned int i = 0; i < p_num_harts; i++) {
            timer_irq[i].init(m_dev, i);
        }

        // Timer IRQ GPIO outputs are now ready for connection
        SCP_INFO(SCMOD) << "Timer IRQ GPIO outputs ready for connection";
    }
};

extern "C" void module_register();
