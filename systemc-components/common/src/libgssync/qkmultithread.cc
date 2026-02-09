/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mutex>
#include <condition_variable>
#include <functional>
#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif
#include <systemc>
#include <pre_suspending_sc_support.h>
#include <tlm>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <async_event.h>
#include <qk_extendedif.h>
#include <qkmultithread.h>
#include <libgsutils.h>
#include <uutils.h>

namespace gs {
/* constantly monitor SystemC and dont let it get ahead of the
   local_time - this is the tlm2.0 rule (h) */
void tlm_quantumkeeper_multithread::timehandler()
{
    std::unique_lock<std::mutex> lock(mutex);

    if (status != RUNNING) {
        // NB must be handled from within timehandler SC_METHOD process
        // Otherwise the sc_unsuspend wont be in the right process
        m_systemc_waiting = false;
        SCP_TRACE(())("Unsuspending (stopped)");
        sc_core::sc_unsuspend_all();
        /*
         * Detach the suspending state here on the SystemC thread (immediate
         * effect) rather than in stop() on the QEMU thread (deferred). This
         * avoids a race where a start()+stop() sequence from the QEMU thread
         * causes the deferred detach to overwrite the deferred attach due to
         * async_event's last-one-wins pending state semantics.
         */
        m_tick.async_detach_suspending();
        return;
    }
    if ((get_current_time() > sc_core::sc_time_stamp())) {
        // UnSuspend SystemC if local time is ahead of systemc time
        m_systemc_waiting = false;
        SCP_TRACE(())("Unsuspending");
        sc_core::sc_unsuspend_all();
        sc_core::sc_time m_quantum = tlm_utils::tlm_quantumkeeper::get_global_quantum();
        m_tick.notify(std::min(get_current_time() - sc_core::sc_time_stamp(), m_quantum));
    } else {
        // Suspend SystemC if SystemC has caught up with our
        // local_time
        m_systemc_waiting = true;
        SCP_TRACE(())("Suspending");
        sc_core::sc_suspend_all();
    }

    cond.notify_all(); // nudge the sync thread, in case it's waiting for us
}

tlm_quantumkeeper_multithread::~tlm_quantumkeeper_multithread()
{
    SigHandler::get().deregister_on_exit_cb(std::string(name()) + ".gs::tlm_quantumkeeper_multithread::stop");
    cond.notify_all();
}

// The quantum keeper should be instanced in SystemC
// but it's functions may be called from other threads
// The QK may be instanced outside of elaboration
tlm_quantumkeeper_multithread::tlm_quantumkeeper_multithread()
    : m_systemc_thread_id(std::this_thread::get_id()), status(NONE), m_tick(false) /* handle attach manually */
{
    SCP_TRACE(())("Constructor");
    sc_core::sc_spawn_options opt;
    opt.spawn_method();
    opt.set_sensitivity(&m_tick);
    opt.dont_initialize();
    sc_core::sc_spawn(sc_bind(&tlm_quantumkeeper_multithread::timehandler, this), "timehandler", &opt);

    reset();
    SigHandler::get().register_on_exit_cb(std::string(name()) + ".gs::tlm_quantumkeeper_multithread::stop",
                                          [this]() { stop(); });
}

bool tlm_quantumkeeper_multithread::is_sysc_thread() const
{
    if (std::this_thread::get_id() == m_systemc_thread_id)
        return true;
    else
        return false;
}

/*
 * Additional functions
 */

void tlm_quantumkeeper_multithread::start(std::function<void()> job)
{
    SCP_TRACE(())("Start");
    status = RUNNING;
    m_tick.async_attach_suspending();
    m_tick.notify(sc_core::SC_ZERO_TIME);
    if (job) {
        std::thread t(job);
        m_worker_thread = std::move(t);
    }
}

void tlm_quantumkeeper_multithread::stop()
{
    if (status != STOPPED) {
        SCP_TRACE(())("Stop");
        {
            std::unique_lock<std::mutex> lock(mutex);
            status = STOPPED;
            m_tick.notify(sc_core::SC_ZERO_TIME);
            cond.notify_all();
            /*
             * Don't call m_tick.async_detach_suspending() here. When called
             * from the QEMU thread, it's deferred and can race with a
             * preceding start()'s deferred attach (last-one-wins). Instead,
             * the timehandler (running on the SystemC thread) detaches
             * immediately when it detects status != RUNNING.
             */
        }

        if (m_worker_thread.joinable()) {
            m_worker_thread.join();
        }
    }
}

/* this may be overloaded to provide a different window */
/* return the time remaining till the next sync point*/
sc_core::sc_time tlm_quantumkeeper_multithread::time_to_sync()
{
    sc_core::sc_time m_quantum = tlm_utils::tlm_quantumkeeper::get_global_quantum();
    sc_core::sc_time q = sc_core::sc_time_stamp() + (m_quantum * 2);
    if (q >= get_current_time()) {
        return q - get_current_time();
    } else {
        return sc_core::SC_ZERO_TIME;
    }
}

/*
 * Overloaded Functions
 */

void tlm_quantumkeeper_multithread::inc(const sc_core::sc_time& t) { m_local_time += t; }

/* NB, if used outside SystemC, SystemC time may vary */
void tlm_quantumkeeper_multithread::set(const sc_core::sc_time& t)
{
    SCP_TRACE(())("Set {}s", t.to_seconds());
    // quietly refuse to move time backwards
    if (t + sc_core::sc_time_stamp() >= m_local_time) {
        m_local_time = t + sc_core::sc_time_stamp(); // NB, we store the absolute time.
    }
    m_tick.notify(sc_core::SC_ZERO_TIME);
}

void tlm_quantumkeeper_multithread::sync()
{
    if (is_sysc_thread()) {
        assert(m_local_time >= sc_core::sc_time_stamp());
        sc_core::sc_time t = m_local_time - sc_core::sc_time_stamp();
        m_tick.notify(sc_core::SC_ZERO_TIME);
        if (status != STOPPED) {
            sc_core::wait(t);
        }
    } else {
        std::unique_lock<std::mutex> lock(mutex);
        /* Wake up the SystemC thread if it's waiting for us to keep up */
        m_tick.notify(sc_core::SC_ZERO_TIME);
        /* Wait for some run budget */
        m_extern_waiting = true;
        while (status == RUNNING && time_to_sync() == sc_core::SC_ZERO_TIME) {
            if (cond.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout) {
                SCP_WARN(())("wait_for timeout");
                m_tick.notify(sc_core::SC_ZERO_TIME);
            }
        }
        m_extern_waiting = false;
    }
}

void tlm_quantumkeeper_multithread::reset()
{
    // As we use absolute time, we reset to the current sc_time
    m_local_time = sc_core::sc_time_stamp();
    m_tick.notify(sc_core::SC_ZERO_TIME);
}

sc_core::sc_time tlm_quantumkeeper_multithread::get_current_time() const { return m_local_time; }

/* NB not thread safe, you're time may vary, you should really be
 * calling this from SystemC */
sc_core::sc_time tlm_quantumkeeper_multithread::get_local_time() const
{
    sc_core::sc_time sc_t = sc_core::sc_time_stamp();
    if (m_local_time >= sc_t)
        return m_local_time - sc_t;
    else
        return sc_core::SC_ZERO_TIME;
}

bool tlm_quantumkeeper_multithread::need_sync()
{
    // By default we recommend to sync, but other sync policies may vary
    return true;
}
} // namespace gs
