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

#include <libqemu-cxx/target/aarch64.h>

#include "libqbox/components/irq-ctrl/armv7m-nvic.h"
#include "libqbox/components/cpu/cpu.h"

class CpuArmCortexM7 : public QemuCpu
{
public:
    static constexpr qemu::Target ARCH = qemu::Target::AARCH64;

    cci::cci_param<bool> p_start_powered_off;
    QemuNvicArmv7m m_nvic;
    cci::cci_param<uint64_t> p_init_nsvtor;

    CpuArmCortexM7(sc_core::sc_module_name name, QemuInstance& inst)
        : QemuCpu(name, inst, "cortex-m7-arm")
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
