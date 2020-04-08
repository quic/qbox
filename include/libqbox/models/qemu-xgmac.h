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

#include <systemc>
#include "tlm.h"

#include <libqemu-cxx/target/arm.h>

#include "gic.h"

class QemuXgmac : public QemuComponent {
public:
    QemuCpu* m_cpu;
    QemuArmGic *m_gic;
    uint64_t m_addr;

protected:
    using QemuComponent::m_obj;

public:
    QemuXgmac(sc_core::sc_module_name name, QemuCpu *cpu,
            /* FIXME */ QemuArmGic *gic,
            /* FIXME */ uint64_t addr)
        : QemuComponent(name, "xgmac")
        , m_cpu(cpu)
        , m_gic(gic)
        , m_addr(addr)
    {
        m_extra_qemu_args.push_back("-netdev");
        m_extra_qemu_args.push_back("user,id=net0");

        cpu->add_to_qemu_instance(this);
    }

    ~QemuXgmac()
    {
    }

    void set_qemu_properties_callback()
    {
        m_obj.set_prop_str("netdev", "net0");
    }

    void set_qemu_instance_callback()
    {
        QemuComponent::set_qemu_instance_callback();

        set_qemu_properties_callback();
    }

    void before_end_of_elaboration()
    {
    }

    void end_of_elaboration()
    {
        QemuComponent::end_of_elaboration();

        /* FIXME: expose socket */
        qemu::CpuArm cpu = qemu::CpuArm(m_cpu->get_qemu_obj());
        qemu::SysBusDevice sbd = qemu::SysBusDevice(get_qemu_obj());
        qemu::MemoryRegion root_mr = sbd.mmio_get_region(0);
        qemu::MemoryRegion mr = m_lib->object_new<qemu::MemoryRegion>();
        mr.init_alias(cpu, "cpu-alias", root_mr, 0, root_mr.get_size());
        m_cpu->m_ases[0].mr.add_subregion(mr, m_addr);

        /* FIXME: implement qemu direct mapping */
        qemu::Device gic_dev(m_gic->get_qemu_obj());
        sbd.connect_gpio_out(0, gic_dev.get_gpio_in(80));
        sbd.connect_gpio_out(1, gic_dev.get_gpio_in(81));
        sbd.connect_gpio_out(2, gic_dev.get_gpio_in(82));
    }
};
