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

#include "libqbox/libqbox.h"

class QemuUart16550 : public QemuComponent {
protected:
    /* FIXME: remove this after refactoring */
    std::vector<QemuCpu *> m_cpus;
    uint64_t m_map_addr;
    uint64_t m_map_size;
    QemuComponent *m_target_irq;
    int m_target_irq_num;

    qemu::Chardev m_chardev;

public:
    QemuUart16550(const sc_core::sc_module_name &n)
        : QemuComponent(n, "serial-mm")
        , m_target_irq(nullptr)
    {
    }

    void set_qemu_properties_callback() {
        qemu::Device dev(get_qemu_obj());

        /* FIXME: hardcoded for now */
        m_obj.set_prop_int("baudbase", 38400000);
        m_obj.set_prop_int("regshift", 2);
        m_chardev = m_lib->chardev_new("uart", "stdio");
        dev.set_prop_chardev("chardev", m_chardev);
    }

    void set_qemu_instance_callback() {
        QemuComponent::set_qemu_instance_callback();
        set_qemu_properties_callback();
    }

    /* FIXME: remove this after refactoring */
    void set_map_addr(uint64_t addr, uint64_t size)
    {
        m_map_addr = addr;
        m_map_size = size;
    }

    /* FIXME: remove this after refactoring */
    void connect_irq_out(QemuComponent &target, int num)
    {
        m_target_irq = &target;
        m_target_irq_num = num;
    }

    /* FIXME: remove this after refactoring */
    void add_cpu(QemuCpu &cpu)
    {
        if (m_cpus.empty()) {
            cpu.add_to_qemu_instance(this);
        }

        m_cpus.push_back(&cpu);
    }

    void end_of_elaboration() {
        QemuComponent::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_obj());
        qemu::MemoryRegion uart_mr(sbd.mmio_get_region(0));

        /* FIXME remove this after refactoring */
        for (auto cpu: m_cpus) {
            qemu::Cpu qemu_cpu(cpu->get_qemu_obj());
            qemu::MemoryRegion alias;

            alias = m_lib->object_new<qemu::MemoryRegion>();
            alias.init_alias(qemu_cpu, "uart16550-alias", uart_mr, 0, uart_mr.get_size());
            cpu->m_ases[0].mr.add_subregion(alias, m_map_addr);
        }

        if (m_target_irq) {
            qemu::Device target(m_target_irq->get_qemu_obj());
            sbd.connect_gpio_out(0, target.get_gpio_in(m_target_irq_num));
        }
    }
};
