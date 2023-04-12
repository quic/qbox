/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Alwalid Salama
 *  Author: Lukas Juenger
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
#include <optional>

#include <libqemu-cxx/target/hexagon.h>

#include <greensocs/libgssync.h>

#include "libqbox/components/cpu/cpu.h"

#define DSP_REV(arch) \
    { #arch, arch##_rev }

class QemuCpuHexagon : public QemuCpu
{
public:
    static constexpr qemu::Target ARCH = qemu::Target::HEXAGON;

    typedef enum {
        v66_rev = 0xa666,
        v68_rev = 0x8d68,
        v69_rev = 0x8c69,
        v73_rev = 0x8c73,
    } Rev_t;

    const std::map<std::string, Rev_t> DSP_REVS = {
        DSP_REV(v66),
        DSP_REV(v68),
        DSP_REV(v69),
        DSP_REV(v73),
    };
    sc_core::sc_vector<QemuTargetSignalSocket> irq_in;

    QemuCpuHexagon(const sc_core::sc_module_name& name, QemuInstance& inst, uint32_t cfgbase,
                   Rev_t rev, uint32_t l2vic_base_addr, uint32_t qtimer_base_addr,
                   uint32_t exec_start_addr)
        : QemuCpu(name, inst, "v67-hexagon")
        , irq_in("irq_in", 8, [](const char* n, int i) { return new QemuTargetSignalSocket(n); })
        , m_cfgbase(cfgbase)
        , m_l2vic_base_addr(l2vic_base_addr)
        , m_qtimer_base_addr(qtimer_base_addr)
        , m_exec_start_addr(exec_start_addr)
        , p_dsp_arch("dsp-arch", "v68", "DSP arch")
        , p_start_powered_off("start_powered_off", false,
                              "Start and reset the CPU "
                              "in powered-off state")
        , p_sched_limit("sched_limit", true, "use sched limit")

    /*
     * We have no choice but to attach-suspend here. This is fixable but
     * non-trivial. It means that the SystemC kernel will never starve...
     */
    {
        for (int i = 0; i < irq_in.size(); ++i) {
            m_external_ev |= irq_in[i]->default_event();
        }
    }

    void before_end_of_elaboration() override {
        // set the parameter config-table-addr 195 hexagon_testboard
        QemuCpu::before_end_of_elaboration();
        qemu::CpuHexagon cpu(get_qemu_dev());

        Rev_t dsp_rev = v68_rev;
        const std::string dsp_arch = p_dsp_arch.get_value();
        auto rev = DSP_REVS.find(dsp_arch);
        if (rev != DSP_REVS.end()) {
            dsp_rev = rev->second;
        }
        cpu.set_prop_int("config-table-addr", m_cfgbase);
        cpu.set_prop_int("dsp-rev", dsp_rev);
        cpu.set_prop_int("l2vic-base-addr", m_l2vic_base_addr);
        cpu.set_prop_int("qtimer-base-addr", m_qtimer_base_addr);
        cpu.set_prop_int("exec-start-addr", m_exec_start_addr);
        cpu.set_prop_bool("start-powered-off", p_start_powered_off);
        // in case of additional reset, this value will be loaded for PC
        cpu.set_prop_int("start-evb", m_exec_start_addr);
        cpu.set_prop_bool("sched-limit", p_sched_limit);
        cpu.set_prop_bool("virtual-platform-mode", true);
    }

    void end_of_elaboration() override {
        QemuCpu::end_of_elaboration();

        for (int i = 0; i < irq_in.size(); ++i) {
            irq_in[i].init(m_dev, i);
        }
    }

protected:
    uint32_t m_cfgbase;
    uint32_t m_l2vic_base_addr;
    uint32_t m_qtimer_base_addr;
    uint32_t m_exec_start_addr;

public:
    cci::cci_param<bool> p_start_powered_off;
    cci::cci_param<bool> p_sched_limit;
    cci::cci_param<std::string> p_dsp_arch;
};
