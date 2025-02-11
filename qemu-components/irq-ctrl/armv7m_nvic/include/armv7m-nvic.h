/*
 * This file is part of libqbox
 * Copyright(c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cci_configuration>
#include <libqemu-cxx/target/aarch64.h>

#include <device.h>
#include <ports/target.h>
#include <ports/qemu-target-signal-socket.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <module_factory_registery.h>

class nvic_armv7m : public QemuDevice
{
protected:
    bool before_end_of_elaboration_done;

public:
    cci::cci_param<uint32_t> p_num_irq;
    cci::cci_param<uint8_t> p_num_prio_bits;
    QemuTargetSocket<> socket;
    sc_core::sc_vector<QemuTargetSignalSocket> irq_in;
    QemuTargetSignalSocket nmi;
    QemuTargetSignalSocket NS_SysTick; // Non secure SysTick
    QemuTargetSignalSocket S_SysTick;  // Secure SysTick
    QemuInitiatorSignalSocket irq_out;

    nvic_armv7m(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : nvic_armv7m(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    nvic_armv7m(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "armv7m_nvic")
        , before_end_of_elaboration_done(false)
        , p_num_irq("num_irq", 64, "Number of external interrupts")
        , p_num_prio_bits("num_prio_bits", 0,
                          "Number of the maximum priority bits that can be used. 0 means to use a reasonable default")
        , socket("mem", inst)
        , irq_in("irq_in", p_num_irq)
        , nmi("nmi")
        , NS_SysTick("NS_SysTick")
        , S_SysTick("S_SysTick")
        , irq_out("irq_out")
    {
    }

    void before_end_of_elaboration() override
    {
        if (before_end_of_elaboration_done) {
            return;
        }

        QemuDevice::before_end_of_elaboration();
        before_end_of_elaboration_done = true;

        /* add the cpu link so we can set it later with set_cpu() */
        qemu::ArmNvic nvic(m_dev);
        nvic.add_cpu_link();

        m_dev.set_prop_int("num-irq", p_num_irq);
        m_dev.set_prop_int("num-prio-bits", p_num_prio_bits);
    }

    void end_of_elaboration() override
    {
        /*
         * At this point, the cpu link must have been set. Otherwise
         * realize will fail.
         *
         * Note: the cpu is used by qemu nvic to configure the nvic
         * depending on cpu features (See hw/intc/armv7m_nvic.c realize
         * function in qemu code).
         */
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);

        /* registers */
        socket.init(sbd, 0);

        /* interrupts */
        for (int i = 0; i < p_num_irq; i++) {
            irq_in[i].init(m_dev, i);
        }

        /* Output lines */
        irq_out.init_sbd(sbd, 0);
        nmi.init_named(m_dev, "NMI", 0);
        NS_SysTick.init_named(m_dev, "systick-trigger", 0);
        S_SysTick.init_named(m_dev, "systick-trigger", 1);
    }
};

extern "C" void module_register();
