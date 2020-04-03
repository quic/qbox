/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps
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

#include "libqbox/libqbox.h"

#define GTIMER_PHYS 0
#define GTIMER_VIRT 1
#define GTIMER_HYP  2
#define GTIMER_SEC  3

#define ARCH_TIMER_VIRT_IRQ   11
#define ARCH_TIMER_S_EL1_IRQ  13
#define ARCH_TIMER_NS_EL1_IRQ 14
#define ARCH_TIMER_NS_EL2_IRQ 10

class QemuArmGic : public QemuComponent {
public:
    sc_core::sc_vector<QemuInPort> spis;

    std::vector<QemuCpu *> m_cores;

    uint32_t m_num_irq;
    uint32_t m_revision;

    const uint32_t GIC_INTERNAL = 32;
    const uint32_t GIC_NR_SGIS = 16;

public:
    QemuArmGic(sc_core::sc_module_name name)
        : QemuComponent(name, "arm_gic")
        , spis("spis")
    {
        m_num_irq = 256;
        m_revision = 2;

        spis.init(m_num_irq, [this](const char *cname, size_t i) { return new QemuInPort(cname, *this, i); });
    }

    void add_core(QemuCpu *core)
    {
        m_cores.push_back(core);
        if (m_cores.size() == 1) {
            m_cores[0]->add_to_qemu_instance(this);
        }
    }

    ~QemuArmGic()
    {
    }

    void set_qemu_properties_callback()
    {
        m_obj.set_prop_int("num-cpu", m_cores.size());
        m_obj.set_prop_int("revision", m_revision);
        m_obj.set_prop_int("num-irq", m_num_irq + GIC_INTERNAL);
    }

    void set_qemu_instance_callback()
    {
        QemuComponent::set_qemu_instance_callback();

        set_qemu_properties_callback();
    }

    void before_end_of_elaboration()
    {
        for (QemuInPort& p : spis) {
            if (!p.get_interface()) {
                sc_core::sc_signal<bool>* stub = new sc_core::sc_signal<bool>(sc_core::sc_gen_unique_name("stub"));
                p.bind(*stub);
            }
        }
    }

    void end_of_elaboration()
    {
        qemu::SysBusDevice busdev;
        qemu::Device gicdev;

        QemuComponent::end_of_elaboration();

        busdev = qemu::SysBusDevice(get_qemu_obj());
        gicdev = qemu::Device(get_qemu_obj());

#if 1
        for (auto &core : m_cores) {
            qemu::CpuArm cpu;
            qemu::MemoryRegion dist_mr;
            qemu::MemoryRegion cpu_mr;
            qemu::MemoryRegion mr;

            cpu = qemu::CpuArm(core->get_qemu_obj());

            dist_mr = busdev.mmio_get_region(0);
            mr = m_lib->object_new<qemu::MemoryRegion>();
            mr.init_alias(cpu, "dist-alias", dist_mr, 0, dist_mr.get_size());
            core->m_ases[0].mr.add_subregion(mr, 0xc8000000);

            cpu_mr = busdev.mmio_get_region(1);
            mr = m_lib->object_new<qemu::MemoryRegion>();
            mr.init_alias(cpu, "cpu-alias", cpu_mr, 0, cpu_mr.get_size());
            core->m_ases[0].mr.add_subregion(mr, 0xc8001000);
        }
#endif

        for (unsigned i = 0; i < m_cores.size(); i++) {
            QemuCpu *core = m_cores[i];
            qemu::CpuArm cpu = qemu::CpuArm(core->get_qemu_obj());

            int ppibase = m_num_irq + i * GIC_INTERNAL + GIC_NR_SGIS;

            int timer_irq[4];
            timer_irq[GTIMER_PHYS] = ARCH_TIMER_NS_EL1_IRQ;
            timer_irq[GTIMER_VIRT] = ARCH_TIMER_VIRT_IRQ;
            timer_irq[GTIMER_HYP]  = ARCH_TIMER_NS_EL2_IRQ;
            timer_irq[GTIMER_SEC]  = ARCH_TIMER_S_EL1_IRQ;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) ((sizeof(x) / sizeof((x)[0])))
#endif
            for (size_t irq = 0; irq < ARRAY_SIZE(timer_irq); irq++) {
                cpu.connect_gpio_out(irq,
                        gicdev.get_gpio_in(ppibase + timer_irq[irq]));
            }

            busdev.connect_gpio_out(i, cpu.get_gpio_in(0));
            busdev.connect_gpio_out(i + 1 * m_cores.size(), cpu.get_gpio_in(1));
            busdev.connect_gpio_out(i + 2 * m_cores.size(), cpu.get_gpio_in(2));
            busdev.connect_gpio_out(i + 3 * m_cores.size(), cpu.get_gpio_in(3));
        }
    }
};


