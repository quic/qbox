/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Lukas Jünger
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

#include <vector>

#include "libqbox/libqbox.h"
#include <libqemu-cxx/target/riscv64.h>

class QemuRiscvSifiveClint : public QemuComponent {
protected:
    /* FIXME: remove this after refactoring */
    std::vector<QemuCpu *> m_cpus;
    uint64_t m_map_addr;
    uint64_t m_map_size;

public:
    QemuRiscvSifiveClint(sc_core::sc_module_name nm)
            : QemuComponent(nm, "riscv.sifive.clint")
    {}

    /* FIXME: remove this after refactoring */
    void add_cpu(QemuCpu &cpu)
    {
        if (m_cpus.empty()) {
            cpu.add_to_qemu_instance(this);
        }

        m_cpus.push_back(&cpu);
    }

    /* FIXME: remove this after refactoring */
    void set_map_addr(uint64_t addr, uint64_t size)
    {
        m_map_addr = addr;
        m_map_size = size;
    }

    void set_qemu_properties_callback() {
        m_obj.set_prop_int("aperture-size", m_map_size);
        m_obj.set_prop_int("num-harts", m_cpus.size());

        /* FIXME: hardcoded for now */
        m_obj.set_prop_int("sip-base", 0);
        m_obj.set_prop_int("timecmp-base", 0x4000);
        m_obj.set_prop_int("time-base", 0xbff8);
        m_obj.set_prop_bool("provide-rdtime", true);
    }

    void set_qemu_instance_callback() {
        QemuComponent::set_qemu_instance_callback();
        set_qemu_properties_callback();
    }

    void end_of_elaboration() {
        QemuComponent::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_obj());
        qemu::MemoryRegion clint_mr(sbd.mmio_get_region(0));

        /* FIXME remove this after refactoring */
        for (auto cpu: m_cpus) {
            qemu::CpuRiscv64 qemu_cpu(cpu->get_qemu_obj());
            qemu::MemoryRegion alias;

            alias = m_lib->object_new<qemu::MemoryRegion>();
            alias.init_alias(qemu_cpu, "clint-alias", clint_mr, 0, clint_mr.get_size());
            cpu->m_ases[0].mr.add_subregion(alias, m_map_addr);
        }
    }
};
