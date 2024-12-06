/*
 * This file is part of libqbox
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_CPU_CPU_H
#define _LIBQBOX_COMPONENTS_CPU_CPU_H

#include <sstream>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <cci_configuration>

#include <libgssync.h>

#include "device.h"
#include "ports/initiator.h"
#include "tlm-extensions/qemu-cpu-hint.h"
#include "ports/qemu-target-signal-socket.h"

class QemuCpu : public QemuDevice, public QemuInitiatorIface
{
protected:
    /*
     * We have a unique copy per CPU of this extension, which is not dynamically allocated.
     * We really don't want the default implementation to call delete on it...
     */
    class QemuCpuHintTlmExtension : public ::QemuCpuHintTlmExtension
    {
    public:
        void free() override
        { /* leave my extension alone, TLM */
        }
    };

    gs::runonsysc m_on_sysc;
    std::shared_ptr<qemu::Timer> m_deadline_timer;
    bool m_coroutines;

    qemu::Cpu m_cpu;

    gs::async_event m_qemu_kick_ev;
    sc_core::sc_event_or_list m_external_ev;
    sc_core::sc_process_handle m_sc_thread; // used for co-routines

    std::atomic<bool> m_signaled;
    std::mutex m_signaled_lock;
    std::condition_variable m_signaled_cond;

    std::shared_ptr<gs::tlm_quantumkeeper_extended> m_qk;
    std::atomic<bool> m_finished = false;
    std::atomic<bool> m_started = false;
    std::mutex m_can_delete;
    QemuCpuHintTlmExtension m_cpu_hint_ext;

    uint64_t m_quantum_ns; // For convenience

    /*
     * Request quantum keeper from instance
     */
    void create_quantum_keeper()
    {
        m_qk = m_inst.create_quantum_keeper();

        if (!m_qk) {
            SCP_FATAL(()) << "qbox : Sync policy unknown";
        }

        m_qk->reset();
    }

    /*
     * Given the quantum keeper nature (synchronous or asynchronous) and the
     * p_icount parameter, we can configure the QEMU instance accordingly.
     */
    void set_coroutine_mode()
    {
        switch (m_qk->get_thread_type()) {
        case gs::SyncPolicy::SYSTEMC_THREAD:
            m_coroutines = true;
            break;

        case gs::SyncPolicy::OS_THREAD:
            m_coroutines = false;
            break;
        }
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
        if (m_inst.get_tcg_mode() != QemuInstance::TCG_SINGLE) {
            std::lock_guard<std::mutex> lock(m_signaled_lock);
            m_signaled = true;
            m_signaled_cond.notify_all();
        } else {
            std::lock_guard<std::mutex> lock(m_inst.g_signaled_lock);
            m_inst.g_signaled = true;
            m_inst.g_signaled_cond.notify_all();
        }
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
        SCP_TRACE(())("QEMU deadline KICK callback");
        if (m_coroutines) {
            if (!m_finished) m_qemu_kick_ev.async_notify();
        } else {
            set_signaled();
        }
    }

    /*
     * Called by the QEMU iothread when the deadline timer expires. We kick the
     * CPU out of its execution loop for it to call the end_of_loop_cb callback.
     * However, we should also handle the case that qemu is currently in 'sync'
     *  - by setting the time here, we will nudge the sync thread.
     */
    void deadline_timer_cb()
    {
        SCP_TRACE(())("QEMU deadline timer callback");
        // All syncing will be done in end_of_loop_cb
        m_cpu.kick();
        // Rearm timer for next time ....
        if (!m_finished) {
            rearm_deadline_timer();

            /* Take this opportunity to set the time */
            int64_t now = m_inst.get().get_virtual_clock();
            sc_core::sc_time sc_t = sc_core::sc_time_stamp();
            if (sc_core::sc_time(now, sc_core::SC_NS) > sc_t) {
                m_qk->set(sc_core::sc_time(now, sc_core::SC_NS) - sc_t);
            }
        }
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
        SCP_TRACE(())("Wait for work");
        m_qk->stop();
        if (m_finished) return;

        if (m_coroutines) {
            m_on_sysc.run_on_sysc([this]() { wait(m_external_ev); });
        } else {
            if (m_inst.get_tcg_mode() != QemuInstance::TCG_SINGLE) {
                std::unique_lock<std::mutex> lock(m_signaled_lock);
                m_signaled_cond.wait(lock, [this] { return m_signaled || m_finished; });
                m_signaled = false;
            } else {
                std::unique_lock<std::mutex> lock(m_inst.g_signaled_lock);
                m_inst.g_signaled_cond.wait(lock, [this] { return m_inst.g_signaled || m_finished; });
                m_inst.g_signaled = false;
            }
        }
        if (m_finished) return;
        SCP_TRACE(())("Have work, running CPU");
        m_qk->start();
    }

    /*
     * Set the deadline timer to trigger at the end of the time budget
     */
    void rearm_deadline_timer()
    {
        // This is a simple "every quantum" tick. Whether the QK makes use of it or not
        // is down to the sync policy
        m_deadline_timer->mod(m_inst.get().get_virtual_clock() + m_quantum_ns);
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

        SCP_TRACE(())("Prepare run");
        if (m_inst.get_tcg_mode() == QemuInstance::TCG_SINGLE) {
            while (!m_inst.can_run() && !m_finished) {
                wait_for_work();
            }
        } else {
            while (!m_cpu.can_run() && !m_finished) {
                if (!m_coroutines) {
                    // In the case of accelerators, allow them to handle signals etc.
                    SCP_TRACE(())("Stopping QK");
                    m_qk->stop();              // Stop the QK, it will be enabled when we next see work to do.
                    break;
                }
                wait_for_work();
            }
        }

        if (m_started) {
            m_cpu.set_soft_stopped(false);
        }
        /*
         * The QEMU CPU loop expect us to enter it with the iothread mutex locked.
         * It is then unlocked when we come back from the CPU loop, in
         * sync_with_kernel().
         */
        m_inst.get().lock_iothread();
    }

    /*
     * Run the CPU loop. Only used in coroutine mode.
     */
    void run_cpu_loop()
    {
        auto last_vclock = m_inst.get().get_virtual_clock();
        m_cpu.loop();
        /*
         * Workaround in icount mode: sometimes, the CPU does not execute
         * on the first call of run_loop(). Give it a second chance.
         */
        for (int i = 0; i < m_inst.number_devices(); i++) {
            if ((m_inst.get().get_virtual_clock() == last_vclock) && (m_cpu.can_run())) {
                m_cpu.loop();
            } else
                break;
        }
    }

    /*
     * Called after a CPU loop run. It synchronizes with the kernel.
     */
    void sync_with_kernel()
    {
        int64_t now = m_inst.get().get_virtual_clock();

        m_cpu.set_soft_stopped(true);

        m_inst.get().unlock_iothread();
        if (!m_coroutines) {
            m_qk->start(); // we may have switched the QK off, so switch it on before setting
        }
        sc_core::sc_time sc_t = sc_core::sc_time_stamp();
        if (sc_core::sc_time(now, sc_core::SC_NS) > sc_t) {
            m_qk->set(sc_core::sc_time(now, sc_core::SC_NS) - sc_t);
        }
        // Important to allow QK to notify itself if it's waiting.
        m_qk->sync();
    }

    /*
     * Callback called when the CPU exits its execution loop. In coroutine
     * mode, we yield here to come back to run_cpu_loop(). In TCG thread mode,
     * we use this hook to synchronize with the kernel.
     */
    void end_of_loop_cb()
    {
        SCP_TRACE(())("End of loop");
        if (m_finished) return;
        if (m_coroutines) {
            m_inst.get().coroutine_yield();
        } else {
            std::lock_guard<std::mutex> lock(m_can_delete);
            sync_with_kernel();
            prepare_run_cpu();
        }
    }

    /*
     * SystemC thread entry when running in coroutine mode.
     */
    void mainloop_thread_coroutine()
    {
        m_cpu.register_thread();

        for (; !m_finished;) {
            prepare_run_cpu();
            run_cpu_loop();
            sync_with_kernel();
        }
    }

public:
    cci::cci_param<unsigned int> p_gdb_port;

    /* The default memory socket. Mapped to the default CPU address space in QEMU */
    QemuInitiatorSocket<> socket;
    TargetSignalSocket<bool> halt;
    TargetSignalSocket<bool> reset;

    SC_HAS_PROCESS(QemuCpu);

    QemuCpu(const sc_core::sc_module_name& name, QemuInstance& inst, const std::string& type_name)
        : QemuDevice(name, inst, (type_name + "-cpu").c_str())
        , halt("halt")
        , reset("reset")
        , m_qemu_kick_ev(false)
        , m_signaled(false)
        , p_gdb_port("gdb_port", 0, "Wait for gdb connection on TCP port <gdb_port>")
        , socket("mem", *this, inst)
    {
        using namespace std::placeholders;

        m_external_ev |= m_qemu_kick_ev;

        auto haltcb = std::bind(&QemuCpu::halt_cb, this, _1);
        halt.register_value_changed_cb(haltcb);
        auto resetcb = std::bind(&QemuCpu::reset_cb, this, _1);
        reset.register_value_changed_cb(resetcb);

        create_quantum_keeper();
        set_coroutine_mode();

        if (!m_coroutines) {
            SC_THREAD(watch_external_ev);
        }

        m_inst.add_dev(this);
    }

    virtual ~QemuCpu()
    {
        end_of_simulation(); // catch the case we exited abnormally
        while (!m_can_delete.try_lock()) {
            m_qk->stop();
        }
        m_inst.del_dev(this);
    }

    // Process shutting down the CPU's at end of simulation, check this was done on destruction.
    // This gives time for QEMU to exit etc.
    void end_of_simulation() override
    {
        if (m_finished) return;
        m_finished = true; // assert before taking lock (for co-routines too)

        if (!m_cpu.valid()) {
            /* CPU hasn't been created yet */
            return;
        }

        if (!m_realized) {
            return;
        }

        m_inst.get().lock_iothread();
        /* Make sure QEMU won't call us anymore */
        m_cpu.clear_callbacks();

        if (m_coroutines) {
            // can't join or wait for sc_event
            m_inst.get().unlock_iothread();
            return;
        }

        /* Unblock it if it's waiting for run budget */
        m_qk->stop();

        /* Unblock the CPU thread if it's sleeping */
        set_signaled();

        /* Unblock it if it's waiting for some I/O to complete */
        socket.cancel_all();

        /* Wait for QEMU to terminate the CPU thread */
        /*
         * Theoretically we should m_cpu.remove_sync(); here, however if QEMU is in the process of an io operation or an
         * exclusive cpu region, it will end up waiting for the io operation to finish (effectively waiting for the
         * SystemC thread, or potentially another CPU that wont get the chance to exit)
         */
        m_cpu.set_unplug(true);
        m_cpu.halt(true);

        m_inst.get().unlock_iothread();
        m_cpu.kick(); // Just in case the CPU is currently in the big lock waiting
    }

    /* NB this is usd to determin if this cpu can run in SINGLE mode
     * for the m_inst.can_run calculation
     */
    bool can_run() override { return m_cpu.can_run(); }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_cpu = qemu::Cpu(m_dev);

        if (m_coroutines) {
            m_sc_thread = sc_core::sc_spawn(std::bind(&QemuCpu::mainloop_thread_coroutine, this));
        }

        socket.init(m_dev, "memory");

        m_cpu.set_soft_stopped(true);

        m_cpu.set_end_of_loop_callback(std::bind(&QemuCpu::end_of_loop_cb, this));
        m_cpu.set_kick_callback(std::bind(&QemuCpu::kick_cb, this));

        m_deadline_timer = m_inst.get().timer_new();
        m_deadline_timer->set_callback(std::bind(&QemuCpu::deadline_timer_cb, this));

        m_cpu_hint_ext.set_cpu(m_cpu);
    }

    void halt_cb(const bool& val)
    {
        SCP_TRACE(())("Halt : {}", val);
        if (!m_finished) {
            if (val) {
                m_deadline_timer->del();
                m_qk->stop();
            } else {
                m_qk->start();
                rearm_deadline_timer();
            }
            m_inst.get().lock_iothread();
            m_cpu.halt(val);
            m_inst.get().unlock_iothread();
            m_qemu_kick_ev.async_notify(); // notify the other thread so that the CPU is allowed to continue
        }
    }
    void reset_cb(const bool& val)
    {
        SCP_TRACE(())("Reset : {}", val);
        if (!m_finished && val) {
            m_qk->start();
            SCP_WARN(())("calling reset");
            m_inst.get().lock_iothread();
            m_cpu.reset();
            m_inst.get().unlock_iothread();
            socket.reset();
            m_qemu_kick_ev.async_notify(); // notify the other thread so that the CPU is allowed to continue
        }
        m_qk->reset();
    }
    virtual void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        if (!p_gdb_port.is_default_value()) {
            std::stringstream ss;
            SCP_INFO(()) << "Starting gdb server on TCP port " << p_gdb_port;
            ss << "tcp::" << p_gdb_port;
            m_inst.get().start_gdb_server(ss.str());
        }
    }

    virtual void start_of_simulation() override
    {
        m_quantum_ns = int64_t(tlm_utils::tlm_quantumkeeper::get_global_quantum().to_seconds() * 1e9);

        QemuDevice::start_of_simulation();
        if (m_inst.get_tcg_mode() == QemuInstance::TCG_SINGLE) {
            if (m_inst.can_run()) {
                m_qk->start();
            }
        }
        if (!m_coroutines) {
            /* Prepare the CPU for its first run and release it */
            m_cpu.set_soft_stopped(false);
            rearm_deadline_timer();
            m_cpu.kick();
        }

        // Have not managed to figure out the root cause of the issue, but the
        // PC is not properly set before running KVM, or it is possibly reset to
        // 0 by some routine. By setting the vcpu as dirty, we trigger pushing
        // registers to KVM just before running it.
        m_cpu.set_vcpu_dirty(true);

        m_started = true;
    }

    /* QemuInitiatorIface  */
    virtual void initiator_customize_tlm_payload(TlmPayload& payload) override
    {
        /* Signal the other end we are a CPU */
        payload.set_extension(&m_cpu_hint_ext);
    }

    virtual void initiator_tidy_tlm_payload(TlmPayload& payload) override { payload.clear_extension(&m_cpu_hint_ext); }

    /*
     * Called by the initiator socket just before a memory transaction.
     * We update our current view of the local time and return it.
     */
    virtual sc_core::sc_time initiator_get_local_time() override
    {
        using sc_core::sc_time;
        using sc_core::SC_NS;

        int64_t vclock_now;

        vclock_now = m_inst.get().get_virtual_clock();
        sc_core::sc_time sc_t = sc_core::sc_time_stamp();
        if (sc_time(vclock_now, SC_NS) > sc_t) {
            m_qk->set(sc_time(vclock_now, SC_NS) - sc_t);
            return m_qk->get_local_time();
        } else {
            return sc_core::SC_ZERO_TIME;
        }
    }

    /*
     * Called after the transaction. We must update our local time view to
     * match t.
     */
    virtual void initiator_set_local_time(const sc_core::sc_time& t) override
    {
        m_qk->set(t);

        if (m_qk->need_sync()) {
            /*
             * Kick the CPU out of its execution loop so that we can sync with
             * the kernel.
             */
            m_cpu.kick();
        }
    }

    /* expose async run interface for DMI invalidation */
    virtual void initiator_async_run(qemu::Cpu::AsyncJobFn job) override
    {
        if (!m_finished) m_cpu.async_run(job);
    }
};

#endif
