/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Luc Michel
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

#ifndef _LIBQBOX_COMPONENTS_CPU_CPU_H
#define _LIBQBOX_COMPONENTS_CPU_CPU_H

#include <sstream>
#include <mutex>

#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <cci_configuration>

#include <greensocs/libgssync.h>

#include "libqbox/components/device.h"

class QemuCpu : public QemuDevice {
protected:
    bool m_coroutines;

    qemu::Cpu m_cpu;

    std::shared_ptr<gs::tlm_quantumkeeper_extended> m_qk;

    /*
     * Create the quantum keeper according to the p_sync_policy parameter, and
     * lock the parameter to prevent any modification.
     */
    void create_quantum_keeper()
    {
        m_qk = gs::tlm_quantumkeeper_factory(p_sync_policy);

        if (!m_qk) {
            SC_REPORT_FATAL("qbox", "Sync policy unknown");
        }

        m_qk->reset();

        /* The p_sync_policy parameter should not be modified anymore */
        p_sync_policy.lock();
    }

    /*
     * Given the quantum keeper nature (synchronous or asynchronous) and the
     * p_icount parameter, we can configure the QEMU instance accordingly.
     */
    void set_qemu_instance_options()
    {
        switch (m_qk->get_thread_type()) {
        case gs::SyncPolicy::SYSTEMC_THREAD:
            m_inst.set_tcg_mode(QemuInstance::TCG_SINGLE_COROUTINE);
            m_coroutines = true;
            break;

        case gs::SyncPolicy::OS_THREAD:
            m_inst.set_tcg_mode(QemuInstance::TCG_MULTI);
            m_coroutines = false;
            break;
        }

        if (p_icount.get_value()) {
            m_inst.set_icount_mode(QemuInstance::ICOUNT_ON, p_icount_mips);
        }

        p_icount.lock();
        p_icount_mips.lock();
    }

public:
    cci::cci_param<bool> p_icount;
    cci::cci_param<int> p_icount_mips;
    cci::cci_param<std::string> p_sync_policy;

    SC_HAS_PROCESS(QemuCpu);

    QemuCpu(const sc_core::sc_module_name &name, QemuInstance &inst,
            const std::string &type_name)
        : QemuDevice(name, inst, (type_name + "-cpu").c_str())
        , p_icount("icount", false, "Enable virtual instruction counter")
        , p_icount_mips("icount-mips", 0, "The MIPS shift value for icount mode (1 insn = 2^(mips) ns)")
        , p_sync_policy("sync-policy", "multithread-quantum", "Synchronization Policy to use")
    {
        create_quantum_keeper();
        set_qemu_instance_options();
    }

    virtual ~QemuCpu()
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_cpu = qemu::Cpu(m_dev);

        m_cpu.set_soft_stopped(true);
    }

    virtual void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        m_qk->start();
    }
};

#endif
