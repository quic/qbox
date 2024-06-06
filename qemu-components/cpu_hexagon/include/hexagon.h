/*
 * This file is part of libqbox
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <functional>
#include <optional>

#include <libqemu-cxx/target/hexagon.h>

#include <module_factory_registery.h>
#include <libgssync.h>

#include <cpu.h>

#define DSP_REV(arch)     \
    {                     \
#arch, arch##_rev \
    }

class qemu_cpu_hexagon : public QemuCpu
{
    cci::cci_broker_handle m_broker;

public:
    static constexpr qemu::Target ARCH = qemu::Target::HEXAGON;

    typedef enum {
        v66_rev = 0xa666,
        v68_rev = 0x8d68,
        v69_rev = 0x8c69,
        v73_rev = 0x8c73,
        v79_rev = 0x8c79,
    } Rev_t;

    const std::map<std::string, Rev_t> DSP_REVS = {
        DSP_REV(v66), DSP_REV(v68), DSP_REV(v69), DSP_REV(v73), DSP_REV(v79),
    };
    sc_core::sc_vector<QemuTargetSignalSocket> irq_in;

    qemu_cpu_hexagon(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : qemu_cpu_hexagon(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    qemu_cpu_hexagon(const sc_core::sc_module_name& name, QemuInstance& inst)
        : QemuCpu(name, inst, "v67-hexagon")
        , m_broker(cci::cci_get_broker())
        , irq_in("irq_in", 8, [](const char* n, int i) { return new QemuTargetSignalSocket(n); })
        , p_cfgbase("config_table_addr", 0xffffffffULL, "config table address")
        , p_l2vic_base_addr("l2vic_base_addr", 0xffffffffULL, "l2vic base address")
        , p_qtimer_base_addr("qtimer_base_addr", 0xffffffffULL, "qtimer base address")
        , p_exec_start_addr("hexagon_start_addr", 0xffffffffULL, "execution start address")
        , p_vp_mode("vp_mode", true, "override the vp_mode for testing")
        , p_dsp_arch("dsp_arch", "v68", "DSP arch")
        , p_start_powered_off("start_powered_off", false,
                              "Start and reset the CPU "
                              "in powered-off state")
        , p_sched_limit("sched_limit", true, "use sched limit")
        , p_paranoid("paranoid_commit_state", false, "enable per-packet checks")
        , p_subsystem_id("subsystem_id", 0, "subsystem id")
        , p_hexagon_num_threads("hexagon_num_threads", 8,
                                "number of hexagon threads") // THREADS_MAX in ./target/hexagon/cpu.h
        , p_isdben_trusted("isdben_trusted", true, "isdben trusted")
        , p_isdben_secure("isdben_secure", true, "isdben secure")
        , p_coproc("coproc", "", "coproc")
        , p_vtcm_base_addr("vtcm_base_addr", 0, "vtcm base address")
        , p_vtcm_size_kb("vtcm_size_kb", 0, "vtcm size in kb")
        , p_num_coproc_instance("num_coproc_instance", 0, "number of coproc instances")
        , p_hvx_contexts("hvx_contexts", 0, "number of HVX contexts")
    /*
     * We have no choice but to attach-suspend here. This is fixable but
     * non-trivial. It means that the SystemC kernel will never starve...
     */
    {
        for (int i = 0; i < irq_in.size(); ++i) {
            m_external_ev |= irq_in[i]->default_event();
        }
    }

    void before_end_of_elaboration() override
    {
        const std::string dsp_arch = p_dsp_arch.get_value();
        set_qom_type(dsp_arch + "-hexagon-cpu");
        // set the parameter config-table-addr 195 hexagon_testboard
        QemuCpu::before_end_of_elaboration();
        qemu::CpuHexagon cpu(get_qemu_dev());

        Rev_t dsp_rev = v68_rev;
        auto rev = DSP_REVS.find(dsp_arch);
        if (rev != DSP_REVS.end()) {
            dsp_rev = rev->second;
        }
        cpu.set_prop_int("config-table-addr", p_cfgbase);
        cpu.set_prop_int("dsp-rev", dsp_rev);
        cpu.set_prop_int("l2vic-base-addr", p_l2vic_base_addr);
        cpu.set_prop_int("qtimer-base-addr", p_qtimer_base_addr);
        cpu.set_prop_int("exec-start-addr", p_exec_start_addr);
        cpu.set_prop_bool("start-powered-off", p_start_powered_off);
        // in case of additional reset, this value will be loaded for PC
        cpu.set_prop_int("start-evb", p_exec_start_addr);
        cpu.set_prop_bool("sched-limit", p_sched_limit);
        cpu.set_prop_bool("virtual-platform-mode", p_vp_mode);
        cpu.set_prop_bool("paranoid-commit-state", p_paranoid);
        cpu.set_prop_int("subsystem-id", p_subsystem_id);
        cpu.set_prop_int("thread-count", p_hexagon_num_threads);
        cpu.set_prop_bool("isdben-trusted", p_isdben_trusted);
        cpu.set_prop_bool("isdben-secure", p_isdben_secure);
        cpu.set_prop_str("coproc", p_coproc.get_value().data());
        cpu.set_prop_int("vtcm-base-addr", p_vtcm_base_addr);
        cpu.set_prop_int("vtcm-size-kb", p_vtcm_size_kb);
        cpu.set_prop_int("num-coproc-instance", p_num_coproc_instance);
        cpu.set_prop_int("hvx-contexts", p_hvx_contexts);
    }

    void end_of_elaboration() override
    {
        QemuCpu::end_of_elaboration();

        for (int i = 0; i < irq_in.size(); ++i) {
            irq_in[i].init(m_dev, i);
        }
    }

public:
    cci::cci_param<uint64_t> p_cfgbase;
    cci::cci_param<uint32_t> p_l2vic_base_addr;
    cci::cci_param<uint32_t> p_qtimer_base_addr;
    cci::cci_param<uint32_t> p_exec_start_addr;
    cci::cci_param<bool> p_vp_mode;
    cci::cci_param<bool> p_start_powered_off;
    cci::cci_param<bool> p_sched_limit;
    cci::cci_param<bool> p_paranoid;
    cci::cci_param<std::string> p_dsp_arch;
    cci::cci_param<uint32_t> p_subsystem_id;
    cci::cci_param<uint32_t> p_hexagon_num_threads;
    cci::cci_param<bool> p_isdben_trusted;
    cci::cci_param<bool> p_isdben_secure;
    cci::cci_param<std::string> p_coproc;
    cci::cci_param<uint64_t> p_vtcm_base_addr;
    cci::cci_param<uint32_t> p_vtcm_size_kb;
    cci::cci_param<uint32_t> p_num_coproc_instance;
    cci::cci_param<uint32_t> p_hvx_contexts;
};

extern "C" void module_register();
