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

class CpuArmCortexM7 : public QemuCpu {
public:
    static constexpr qemu::Target ARCH = qemu::Target::AARCH64;

    class QemuNvicArmv7m : public QemuDevice {
    protected:
        bool before_end_of_elaboration_done;

    public:
        cci::cci_param<unsigned int> p_num_irq;
        QemuTargetSocket<> socket;
        sc_core::sc_vector<QemuTargetSignalSocket> irq_in;

        QemuNvicArmv7m(const sc_core::sc_module_name& n, QemuInstance& inst)
            : QemuDevice(n, inst, "armv7m_nvic")
            , before_end_of_elaboration_done(false)
            , p_num_irq("num-irq", 64, "Number of external interrupts")
            , socket("mem", inst)
            , irq_in("irq-in", p_num_irq)
        {
        }

        void before_end_of_elaboration() override
        {
            if (before_end_of_elaboration_done) {
                return;
            }
            before_end_of_elaboration_done = true;

            QemuDevice::before_end_of_elaboration();

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

public:
    cci::cci_param<bool> p_start_powered_off;
    QemuNvicArmv7m m_nvic;

    CpuArmCortexM7(sc_core::sc_module_name name, QemuInstance& inst)
        : QemuCpu(name, inst, "cortex-m7-arm")
        , m_nvic("nvic", inst)
        , p_start_powered_off("start-powered-off", false, "Start and reset the CPU "
                                                          "in powered-off state")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuArm cpu(m_dev);

        cpu.add_nvic_link();
        cpu.set_prop_bool("start-powered-off", p_start_powered_off);

        /* ensure the nvic is also created */
        m_nvic.before_end_of_elaboration();

        /* setup cpu&nvic links so that we can realize both objects */
        qemu::Device nvic = m_nvic.get_qemu_dev();
        cpu.set_prop_link("nvic", nvic);
        nvic.set_prop_link("cpu", cpu);
    }
};
