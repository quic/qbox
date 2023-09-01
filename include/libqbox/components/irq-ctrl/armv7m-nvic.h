/*
 *  This file is part of libqbox
 * Copyright(c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cci_configuration>

#include "libqbox/components/device.h"
#include "libqbox/ports/target.h"
#include "libqbox/ports/target-signal-socket.h"

class QemuNvicArmv7m : public QemuDevice
{
protected:
    bool before_end_of_elaboration_done;

public:
    cci::cci_param<unsigned int> p_num_irq;
    QemuTargetSocket<> socket;
    sc_core::sc_vector<QemuTargetSignalSocket> irq_in;

    QemuNvicArmv7m(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "armv7m_nvic")
        , before_end_of_elaboration_done(false)
        , p_num_irq("num_irq", 64, "Number of external interrupts")
        , socket("mem", inst)
        , irq_in("irq_in", p_num_irq)
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
    }
};