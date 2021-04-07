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
#include <condition_variable>

#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <cci_configuration>

#include <greensocs/libgssync.h>

#include "libqbox/components/device.h"
#include "libqbox/ports/initiator.h"
#include "libqbox/tlm-extensions/qemu-cpu-hint.h"

class QemuCpu : public QemuDevice, public QemuInitiatorIface {
protected:
    gs::RunOnSysC m_on_sysc;
    std::shared_ptr<qemu::Timer> m_deadline_timer;
    bool m_coroutines;

    qemu::Cpu m_cpu;

    gs::async_event m_qemu_kick_ev;
    sc_core::sc_event_or_list m_external_ev;

    bool m_signaled;
    std::mutex m_signaled_lock;
    std::condition_variable m_signaled_cond;

    int64_t m_last_vclock;

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


    /*
     * ---- CPU loop related methods ----
     */

    /*
     * Called by watch_external_ev and kick_cb in MTTCG mode. This keeps track
     * of an external event in case the CPU thread just released the iothread
     * and is going to call wait_for_work. This is needed to avoid missing an
     * event and going to sleep while we should effectively wake-up.
     *
     * The coroutine mode does not use this method and use the SystemC kernel
     * as a mean of synchronization. If an asynchronous event is triggered
     * while the CPU thread go to sleep, the fact that the CPU thread is also
     * the SystemC thread will ensure correct ordering of the events.
     */
    void set_signaled()
    {
        assert(!m_coroutines);

        std::lock_guard<std::mutex> lock(m_signaled_lock);
        m_signaled = true;
        m_signaled_cond.notify_all();
    }

    /*
     * SystemC thread watching the m_external_ev event list. Only used in MTTCG
     * mode.
     */
    void watch_external_ev()
    {
        for (;;) {
            wait(m_external_ev);
            set_signaled();
        }
    }

    /*
     * Called when the CPU is kicked. We notify the corresponding async event
     * to wake the CPU up if it was sleeping waiting for work.
     */
    void kick_cb()
    {
        if (m_coroutines) {
            m_qemu_kick_ev.async_notify();
        } else {
            set_signaled();
        }
    }

    /*
     * Called by the QEMU iothread when the deadline timer expires. We kick the
     * CPU out of its execution loop for it to call the end_of_loop_cb callback.
     */
    void deadline_timer_cb()
    {
        m_cpu.kick();
    }

    /*
     * The CPU does not have work anymore. Pause the CPU thread until we have
     * some work to do.
     *
     * - In coroutine mode, this method runs a wait on the SystemC kernel,
     *   waiting for the m_external_ev list.
     * - In MTTCG mode, we wait on the m_signaled_cond condition, signaled when
     *   set_signaled is called.
     */
    void wait_for_work()
    {
        m_qk->stop();
        if (m_coroutines) {
            m_on_sysc.run_on_sysc([this] () { wait(m_external_ev); });
        } else {
            std::unique_lock<std::mutex> lock(m_signaled_lock);
            m_signaled_cond.wait(lock, [this] { return m_signaled; });
            m_signaled = false;
        }
        m_qk->start();
    }

    /*
     * Set the deadline timer to trigger at the end of the time budget
     */
    void rearm_deadline_timer()
    {
        sc_core::sc_time run_budget;
        int64_t budget_ns, next_dl_ns;

        run_budget = m_qk->time_to_sync();

        budget_ns = int64_t(run_budget.to_seconds() * 1e9);

        m_last_vclock = m_inst.get().get_virtual_clock();
        next_dl_ns = m_last_vclock + budget_ns;

        m_deadline_timer->mod(next_dl_ns);
    }

    /*
     * Called before running the CPU. Lock the BQL and set the deadline timer
     * to not run beyond the time budget.
     */
    void prepare_run_cpu()
    {
        /*
         * The QEMU CPU loop expect us to enter it with the iothread mutex locked.
         * It is then unlocked when we come back from the CPU loop, in
         * sync_with_kernel().
         */
        m_inst.get().lock_iothread();

        while (!m_cpu.can_run()) {
            m_inst.get().unlock_iothread();
            wait_for_work();
            m_inst.get().lock_iothread();
        }

        m_cpu.set_soft_stopped(false);

        rearm_deadline_timer();
    }

    /*
     * Run the CPU loop. Only used in coroutine mode.
     */
    void run_cpu_loop()
    {
        m_cpu.loop();

        /*
         * Workaround in icount mode: sometimes, the CPU does not execute
         * on the first call of run_loop(). Give it a second chance.
         */
        if ((m_inst.get().get_virtual_clock() == m_last_vclock)
            && (m_cpu.can_run())) {
            m_cpu.loop();
        }
    }

    /*
     * Called after a CPU loop run. It synchronizes with the kernel.
     */
    void sync_with_kernel()
    {
        sc_core::sc_time elapsed;
        int64_t now = m_inst.get().get_virtual_clock();

        m_deadline_timer->del();
        m_cpu.set_soft_stopped(true);

        m_inst.get().unlock_iothread();

        elapsed = sc_core::sc_time(now - m_last_vclock, sc_core::SC_NS);
        m_qk->inc(elapsed);

        m_qk->sync();
    }

    /*
     * Callback called when the CPU exits its execution loop. In coroutine
     * mode, we yield here to come back to run_cpu_loop(). In TCG thread mode,
     * we use this hook to synchronize with the kernel.
     */
    void end_of_loop_cb()
    {
        if (sc_core::sc_get_status() < sc_core::SC_RUNNING) {
            /* Simulation hasn't started yet. */
            return;
        }

        if (m_coroutines) {
            m_inst.get().coroutine_yield();
        } else {
            sync_with_kernel();
            prepare_run_cpu();
        }
    }

    /*
     * SystemC thread entry when running in coroutine mode.
     */
    void mainloop_thread_coroutine()
    {
        if (m_coroutines) {
            m_cpu.register_thread();
        }

        for (;;) {
            prepare_run_cpu();
            run_cpu_loop();
            sync_with_kernel();
        }
    }

public:
    cci::cci_param<bool> p_icount;
    cci::cci_param<int> p_icount_mips;
    cci::cci_param<unsigned int> p_gdb_port;
    cci::cci_param<std::string> p_sync_policy;

    /* The default memory socket. Mapped to the default CPU address space in QEMU */
    QemuInitiatorSocket<> socket;

    SC_HAS_PROCESS(QemuCpu);

    QemuCpu(const sc_core::sc_module_name &name, QemuInstance &inst,
            const std::string &type_name)
        : QemuDevice(name, inst, (type_name + "-cpu").c_str())
        , m_qemu_kick_ev(false)
        , m_signaled(false)
        , p_icount("icount", false, "Enable virtual instruction counter")
        , p_icount_mips("icount-mips", 0, "The MIPS shift value for icount mode (1 insn = 2^(mips) ns)")
        , p_gdb_port("gdb-port", 0, "Wait for gdb connection on TCP port <gdb_port>")
        , p_sync_policy("sync-policy", "multithread-quantum", "Synchronization Policy to use")
        , socket("mem", *this, inst)
    {
        m_external_ev |= m_qemu_kick_ev;

        create_quantum_keeper();
        set_qemu_instance_options();

        if (!m_coroutines) {
            SC_THREAD(watch_external_ev);
        }
    }

    virtual ~QemuCpu()
    {
        if (!m_cpu.valid()) {
            /* CPU hasn't been created yet */
            return;
        }

        if (m_coroutines) {
            /* No thread to join */
            return;
        }

        m_inst.get().lock_iothread();
        m_cpu.clear_callbacks();
        m_inst.get().unlock_iothread();

        /*
         * We can't use m_cpu.remove_sync() here as it locks the BQL, and join
         * the CPU thread. If the CPU thread is in an asynchronous SystemC job
         * waiting to be cancelled (because we're at end of simulation), then
         * we'll deadlock here. Instead, set set_unplug to true so that the CPU
         * thread will eventually quit, but do not wait for it to do so.
         */
        m_cpu.set_unplug(true);
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_cpu = qemu::Cpu(m_dev);

        if (m_coroutines) {
            sc_core::sc_spawn(std::bind(&QemuCpu::mainloop_thread_coroutine, this));
        }

        socket.init(m_dev, "memory");

        m_cpu.set_soft_stopped(true);

        m_cpu.set_end_of_loop_callback(std::bind(&QemuCpu::end_of_loop_cb, this));
        m_cpu.set_kick_callback(std::bind(&QemuCpu::kick_cb, this));

        m_deadline_timer = m_inst.get().timer_new();
        m_deadline_timer->set_callback(std::bind(&QemuCpu::deadline_timer_cb,
                                                 this));
    }

    virtual void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        if (!p_gdb_port.is_default_value()) {
            std::stringstream ss;
            std::cout << "Starting gdb server on TCP port " << p_gdb_port << "\n";
            ss << "tcp::" << p_gdb_port;
            m_inst.get().start_gdb_server(ss.str());
        }

        m_qk->start();
    }

    virtual void start_of_simulation() override
    {
        QemuDevice::start_of_simulation();

        if (!m_coroutines) {
            /* Prepare the CPU for its first run and release it */
            m_cpu.set_soft_stopped(false);
            rearm_deadline_timer();
            m_cpu.kick();
        }
    }

    /* QemuInitiatorIface  */
    virtual void initiator_customize_tlm_payload(TlmPayload &payload) override
    {
        /* Signal the other end we are a CPU */
        payload.set_extension(new QemuCpuHintTlmExtension(m_cpu));
    }

    /*
     * Called by the initiator socket just before a memory transaction.
     * We update our current view of the local time and return it.
     */
    virtual sc_core::sc_time initiator_get_local_time() override
    {
        using sc_core::sc_time;
        using sc_core::SC_NS;

        int64_t vclock_now;
        sc_time vclock_delta;

        vclock_now = m_inst.get().get_virtual_clock();
        vclock_delta = sc_time(vclock_now - m_last_vclock, SC_NS);

        m_qk->inc(vclock_delta);
        return m_qk->get_local_time();
    }

    /*
     * Called after the transaction. We must update our local time view to
     * match t.
     */
    virtual void initiator_set_local_time(const sc_core::sc_time &t) override
    {
        m_qk->set(t);

        if (m_qk->need_sync()) {
            /*
             * Kick the CPU out of its execution loop so that we can sync with
             * the kernel.
             */
            m_last_vclock = m_inst.get().get_virtual_clock();
            m_cpu.kick();
        } else {
            /*
             * FIXME: We should in theory update the deadline timer here to
             * account for time that has passed during the memory transaction.
             * Unfortunatly this is quite costly in icount mode and makes the
             * simulation somewhat unresponsive in certain conditions. It could
             * be worth investigate to know where this performance penalty is
             * comming from and see if it is fixable. For now we just ignore
             * the time that passed in the transaction w.r.t. the last deadline
             * we setup in QEMU.
             */
#if 0
            /* Update the deadline timer with this new local time */
            rearm_deadline_timer();
#else
            m_last_vclock = m_inst.get().get_virtual_clock();
#endif
        }
    }

};


#endif
