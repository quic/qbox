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

class QemuSdhci : public QemuComponent {
public:
    QemuCpu* m_cpu;
    QemuArmGic *m_gic;
    uint64_t m_addr;

protected:
    using QemuComponent::m_obj;

public:
    QemuSdhci(sc_core::sc_module_name name, QemuCpu *cpu,
            /* FIXME */ QemuArmGic *gic,
            /* FIXME */ uint64_t addr,
            /* FIXME */ const char *file)
        : QemuComponent(name, "generic-sdhci")
        , m_cpu(cpu)
        , m_gic(gic)
        , m_addr(addr)
    {
        std::stringstream ss;
        ss << "if=sd,id=drive0,file=" << file << ",format=raw";
        m_extra_qemu_args.push_back("-drive");
        m_extra_qemu_args.push_back(ss.str());

        cpu->add_to_qemu_instance(this);
    }

    ~QemuSdhci()
    {
    }

    void set_qemu_properties_callback()
    {
        m_obj.set_prop_int("sd-spec-version", 2);
        m_obj.set_prop_int("capareg", 0x69ec0080);
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

        /* TODO: see sdhci-dma in QEMU */

        /* FIXME: implement qemu direct mapping */
        qemu::Device gic_dev(m_gic->get_qemu_obj());
        sbd.connect_gpio_out(0, gic_dev.get_gpio_in(100));

        /* create sdcard and backend */
        qemu::Device dev = qemu::Device(get_qemu_obj());
        qemu::Object cardobj = m_lib->object_new(("sd-card"));
        qemu::Device carddev(cardobj);
        carddev.set_parent_bus(dev.get_child_bus("sd-bus"));
        carddev.set_prop_str("drive", "drive0");
        carddev.set_prop_bool("realized", true);
    }
};
