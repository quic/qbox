/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Greensocs
 *
 *  Authors: Clement Deschamps
 *           Damien Hedde
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

#include <libqemu-cxx/target/riscv.h>

class QemuRiscvIbexPlic : public QemuComponent {
    sc_core::sc_signal<bool> sig_irq0;
public:
    sc_core::sc_vector<QemuInPort> irqs;

    QemuCpu* m_cpu;

    uint32_t m_num_sources;

protected:
    using QemuComponent::m_obj;

public:
    QemuRiscvIbexPlic(sc_core::sc_module_name name, QemuCpu *cpu, uint32_t num_sources)
        : QemuComponent(name, "ibex-plic")
        , sig_irq0("irq0_stub")
        , irqs("irq")
        , m_cpu(cpu)
        , m_num_sources(num_sources)
    {
        irqs.init(m_num_sources, [this](const char *cname, size_t i) { return new QemuInPort(cname, *this, i); });
        // irq0 must tied to zero, so plug it here
        irqs[0](sig_irq0);

        cpu->add_to_qemu_instance(this);
    }

    ~QemuRiscvIbexPlic()
    {
    }

    void set_qemu_properties_callback()
    {
        m_obj.set_prop_int("num-sources", m_num_sources);
    }

    void set_qemu_instance_callback()
    {
        QemuComponent::set_qemu_instance_callback();

        set_qemu_properties_callback();

        //Note: cpus link are automatically set in qemu
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

        qemu::CpuRiscv cpu = qemu::CpuRiscv(m_cpu->get_qemu_obj());
        qemu::SysBusDevice sbd = qemu::SysBusDevice(get_qemu_obj());
        qemu::MemoryRegion mr = sbd.mmio_get_region(0);
        m_cpu->m_ases[0].mr.add_subregion(mr, 0x40090000);
    }
};


