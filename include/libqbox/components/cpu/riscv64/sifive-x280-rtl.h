/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Alwalid Salama
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
#include <functional>

#include <libqemu-cxx/target/sifive-x280.h>

#include <greensocs/libgssync.h>

#include "libqbox/components/cpu/cpu.h"

#define SIFIVE_X280_RTL_REV1_SUBSYSTEM_PBUS_CLOCK_FREQ   32500000

class QemuCpuSifiveX280: public QemuCpu {

protected:
    uint64_t m_hartid;
    gs::async_event m_irq_ev;

    void mip_update_cb(uint32_t value)
    {
        if (value) {
            m_irq_ev.notify();
        }
    }

public:
    QemuCpuSifiveX280(const sc_core::sc_module_name &name,
                   QemuInstance &inst, const char *model,
                   uint64_t hartid)
        : QemuCpu(name, inst, std::string(model) + "-riscv")
        , m_hartid(hartid)
          /*
           * We have no choice but to attach-suspend here. This is fixable but
           * non-trivial. It means that the SystemC kernel will never starve...
           */
        , m_irq_ev(true)
    {
        m_external_ev |= m_irq_ev;
    }


    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuSifive cpu(get_qemu_dev());
        cpu.set_prop_int("hartid", m_hartid);
        cpu.set_prop_int("timebase-frequency", SIFIVE_X280_RTL_REV1_SUBSYSTEM_PBUS_CLOCK_FREQ);
        cpu.set_prop_int("#size-cells", 0x0);
        cpu.set_prop_int("#address-cells", 0x1);
        cpu.set_prop_str("mmu-type", "riscv,sv48");
        //cpu.set_prop_str("riscv,isa",cpu_isa); to be define
        //qemu_fdt_setprop(fdt, cpu_name, "compatible", cpu_comp,sizeof(cpu_comp)); ??
        cpu.set_prop_str("status", "okay");
        //cpu.set_prop_int("reg", cpu); // check with mark
        //qemu_fdt_setprop_cell(fdt, cpu_name, "reg", cpu);
        cpu.set_prop_str("device_type", "cpu");
        cpu.set_prop_int("d-cache-block-size", 64);
        cpu.set_prop_int("d-cache-sets", 128);
        cpu.set_prop_int("d-cache-size", 32768);
        cpu.set_prop_int("d-tlb-sets", 1);
        cpu.set_prop_int("d-tlb-size", 64);
        cpu.set_prop_int("hardware-exec-breakpoint-count", 4);
        cpu.set_prop_int("hwpf-distanceBits", 6);
        cpu.set_prop_int("hitCacheThrdBits", 5);
        cpu.set_prop_int("hwpf-hitMSHRThrdBits", 4);
        cpu.set_prop_int("hwpf-l2pfPoolSize", 10);
        cpu.set_prop_int("hwpf-nIssQEnt", 6);
        cpu.set_prop_int("hwpf-nPrefetchQueueEntries", 8);
        cpu.set_prop_int("hwpf-nStreams", 8);
        cpu.set_prop_int("hwpf-qFullnessThrdBits", 4);
        cpu.set_prop_int("hwpf-windowBits", 6);
        cpu.set_prop_int("i-cache-block-size", 64);
        cpu.set_prop_int("i-cache-sets", 128);
        cpu.set_prop_int("i-cache-size", 32768);
        cpu.set_prop_int("i-tlb-sets", 1);
        cpu.set_prop_int("i-tlb-size", 64);
        //cpu.set_prop_int("next-level-cache", );// check with mark
        //cpu.set_prop_int("sifive,buserror", );// check with mark
        cpu.set_prop_int("clock-frequency", 0);
        cpu.set_prop_int("timebase-frequency", SIFIVE_X280_RTL_REV1_SUBSYSTEM_PBUS_CLOCK_FREQ);
        //cpu.set_prop_int("phandle", );// check with mark
        cpu.set_prop_str("compatible", "riscv,cpu-intc");
        cpu.set_prop_bool("interrupt-controller", NULL);// ??
        cpu.set_prop_int("#interrupt-cells", 1);
        //cpu.set_mip_update_callback(std::bind(&QemuCpuSifiveX280::mip_update_cb, this, std::placeholders::_1));
    }


// properties to be set from main.cc
};
