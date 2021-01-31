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

#include <string>
#include <cassert>

#include "libqbox/libqbox.h"
#include <libqemu-cxx/target/riscv64.h>

class QemuRiscvSifivePlic : public QemuComponent {
protected:
    std::string m_hart_config;
    int m_num_sources;

    /* FIXME: remove this after refactoring */
    std::vector<QemuCpu *> m_cpus;
    uint64_t m_map_addr;
    uint64_t m_map_size;

public:
    sc_core::sc_vector<QemuInPort> irq_in;

public:
    QemuRiscvSifivePlic(sc_core::sc_module_name nm, int num_sources)
        : QemuComponent(nm, "riscv.sifive.plic")
        , m_num_sources(num_sources)
    {
        irq_in.init(num_sources,
                    [this](const char *cname, size_t i) {
                        return new QemuInPort(cname, *this, i);
                    });
    }

    /**
     * Add CPU to the PLIC configuration
     *
     * Increase the number of connected CPUs to this PLIC by one.
     *
     * @cpu: the CPU to add   FIXME: remove this parameter after refactoring
     * @mode The configuration mode of this CPU. Can be a combination of at
     *       least one of the following letters:
     *          U: user mode
     *          S: supervisor mode
     *          H: hypervisor mode
     *          M: machine mode
     */
    void add_cpu(QemuCpu &cpu, const char *mode)
    {
        const char *m = mode;

        assert(*mode);

        while (*m) {
            switch (*m) {
            case 'U':
            case 'S':
            case 'H':
            case 'M':
                break;

            default:
                assert(0 && "invalid PLIC mode character");
            }

            m++;
        }

        if (m_hart_config.size()) {
            m_hart_config += ',';
        } else {
            cpu.add_to_qemu_instance(this);
        }

        m_cpus.push_back(&cpu);

        m_hart_config += mode;
    }

    /* FIXME: remove this after refactoring */
    void set_map_addr(uint64_t addr, uint64_t size)
    {
        m_map_addr = addr;
        m_map_size = size;
    }

    void set_qemu_properties_callback() {
        m_obj.set_prop_str("hart-config", m_hart_config.c_str());
        m_obj.set_prop_int("num-sources", m_num_sources);
        m_obj.set_prop_int("aperture-size", m_map_size);

        /* FIXME: hardcoded for now */
        m_obj.set_prop_int("num-priorities", 7);
        m_obj.set_prop_int("priority-base", 0x04);
        m_obj.set_prop_int("pending-base", 0x1000);
        m_obj.set_prop_int("enable-base", 0x2000);
        m_obj.set_prop_int("enable-stride", 0x80);
        m_obj.set_prop_int("context-base", 0x200000);
        m_obj.set_prop_int("context-stride", 0x1000);
    }

    void set_qemu_instance_callback() {
        QemuComponent::set_qemu_instance_callback();
        set_qemu_properties_callback();
    }

    void end_of_elaboration() {
        QemuComponent::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_obj());
        qemu::MemoryRegion plic_mr(sbd.mmio_get_region(0));

        /* FIXME remove this after refactoring */
        for (auto cpu: m_cpus) {
            qemu::CpuRiscv64 qemu_cpu(cpu->get_qemu_obj());
            qemu::MemoryRegion alias;

            alias = m_lib->object_new<qemu::MemoryRegion>();
            alias.init_alias(qemu_cpu, "plic-alias", plic_mr, 0, plic_mr.get_size());
            cpu->m_ases[0].mr.add_subregion(alias, m_map_addr);
        }
    }
};
