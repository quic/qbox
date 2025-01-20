/*
 * This file is part of libqbox
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_ICOUNT_PLUGIN_H
#define _LIBQBOX_COMPONENTS_ICOUNT_PLUGIN_H

#include <cassert>
#include <systemc>

#include <cci_configuration>

#include <cciutils.h>

#include <scp/report.h>
#include "libqemu-plugin.h"

class icount_plugin : public LibQemuPlugin
{
    SCP_LOGGER();

private:
    bool sync_time;
    uint64_t max_insn_per_second;
    uint64_t max_insn_per_quantum;
    const void* time_handle;

public:
    typedef struct {
        uint64_t total_insn;
        uint64_t quantum_insn;
    } vCPUTime;

    qemu_plugin_scoreboard* vcpus;
    uint64_t n_cpus;

    icount_plugin(const sc_core::sc_module_name& nm, qemu::LibQemu& inst)
        : LibQemuPlugin(nm, inst), sync_time(false), max_insn_per_second(1000 * 1000 * 1000)
    {
        n_cpus = 0;
    }
    void set_sync_time(bool value) { sync_time = value; }
    void set_max_insn_per_second(uint64_t value) { max_insn_per_second = value; }
    inline void update_total_insn(vCPUTime* vcpu)
    {
        vcpu->total_insn += vcpu->quantum_insn;
        vcpu->quantum_insn = 0;
    }

    void vcpu_init(qemu_plugin_id_t id, unsigned int cpu_index) override
    {
        vCPUTime* vcpu = static_cast<vCPUTime*>(m_inst.p_api.qemu_plugin_scoreboard_find(vcpus, cpu_index));
        vcpu->total_insn = 0;
        vcpu->quantum_insn = 0;
        n_cpus += 1;
    }

    void vcpu_exit(qemu_plugin_id_t id, unsigned int cpu_index) override
    {
        vCPUTime* vcpu = static_cast<vCPUTime*>(m_inst.p_api.qemu_plugin_scoreboard_find(vcpus, cpu_index));
        update_total_insn(vcpu);
    }

    void vcpu_tb_exec_cond(unsigned int cpu_index, void* udata) override
    {
        vCPUTime* vcpu = static_cast<vCPUTime*>(m_inst.p_api.qemu_plugin_scoreboard_find(vcpus, cpu_index));
        assert(vcpu->quantum_insn >= max_insn_per_quantum);
        update_total_insn(vcpu);
        udata = this;
    }

    void vcpu_tb_trans(qemu_plugin_id_t id, qemu_plugin_tb* tb) override
    {
        size_t n_insns = m_inst.p_api.qemu_plugin_tb_n_insns(tb);
        qemu_plugin_u64 quantum_insn = qemu_plugin_scoreboard_u64_in_struct(vcpus, vCPUTime, quantum_insn);

        /* count (and eventually trap) once per tb */
        m_inst.p_api.qemu_plugin_register_vcpu_tb_exec_inline_per_vcpu(tb, QEMU_PLUGIN_INLINE_ADD_U64, quantum_insn,
                                                                       n_insns);

        enable_vcpu_tb_exec_cond(tb, &bridge_vcpu_tb_exec_cond, QEMU_PLUGIN_CB_NO_REGS, QEMU_PLUGIN_COND_GE,
                                 quantum_insn, max_insn_per_quantum, this);
    }

    ~icount_plugin()
    {
        if (sync_time) {
            m_inst.p_api.qemu_plugin_scoreboard_free(vcpus);
        }
    }

    void end_of_elaboration() override
    {
        SCP_DEBUG(()) << "is_plugin_enabled: " << sync_time;

        if (sync_time) {
            LibQemuPlugin::end_of_elaboration();

            vcpus = m_inst.p_api.qemu_plugin_scoreboard_new(sizeof(vCPUTime));

            max_insn_per_quantum = static_cast<uint64_t>(std::floor(
                max_insn_per_second * double(tlm_utils::tlm_quantumkeeper::get_global_quantum().to_seconds())));

            time_handle = m_inst.p_api.qemu_plugin_request_time_control();
            assert(time_handle);
            enable_vcpu_tb_trans(id, &bridge_vcpu_tb_trans);
            enable_vcpu_init(id, &bridge_vcpu_init);
            enable_vcpu_exit(id, &bridge_vcpu_exit);
        }
    }
};

#endif //_LIBQBOX_COMPONENTS_ICOUNT_PLUGIN_H
