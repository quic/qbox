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

class QemuArmNvic : public QemuComponent {
public:
    sc_core::sc_vector<QemuInPort> irqs;

    QemuCpu* m_cpu;

    uint32_t m_num_irq;

protected:
    using QemuComponent::m_obj;

public:
    QemuArmNvic(sc_core::sc_module_name name, QemuCpu *cpu, uint32_t num_irq)
        : QemuComponent(name, "armv7m_nvic")
        , irqs("irq")
        , m_cpu(cpu)
        , m_num_irq(num_irq)
    {
        irqs.init(m_num_irq, [this](const char *cname, size_t i) { return new QemuInPort(cname, *this, i); });

        cpu->add_to_qemu_instance(this);
    }

    ~QemuArmNvic()
    {
    }

    void set_qemu_properties_callback()
    {
        m_obj.set_prop_int("num-irq", m_num_irq);
    }

    void set_qemu_instance_callback()
    {
        QemuComponent::set_qemu_instance_callback();

        set_qemu_properties_callback();

        qemu::ArmNvic nvic(m_obj);
        nvic.add_cpu_link();
        m_obj.set_prop_link("cpu", m_cpu->get_qemu_obj());
    }

    void before_end_of_elaboration()
    {
        for (QemuInPort& p : irqs) {
            if (!p.get_interface()) {
                sc_core::sc_signal<bool>* stub = new sc_core::sc_signal<bool>(sc_core::sc_gen_unique_name("stub"));
                p.bind(*stub);
            }
        }
    }

    void end_of_elaboration()
    {
        QemuComponent::end_of_elaboration();

        qemu::CpuArm cpu = qemu::CpuArm(m_cpu->get_qemu_obj());
        qemu::SysBusDevice sbd = qemu::SysBusDevice(get_qemu_obj());
        qemu::MemoryRegion root_mr = sbd.mmio_get_region(0);
        qemu::MemoryRegion mr = m_lib->object_new<qemu::MemoryRegion>();
        mr.init_alias(cpu, "cpu-alias", root_mr, 0, root_mr.get_size());
        m_cpu->m_ases[0].mr.add_subregion(mr, 0xe000e000);

        qemu::Gpio cpu_irq = cpu.get_gpio_in(0);
        sbd.connect_gpio_out(0, cpu_irq);
    }
};


