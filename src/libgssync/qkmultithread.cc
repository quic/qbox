/*
 * Copyright (c) 2022 GreenSocs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <mutex>
#include <condition_variable>
#include <functional>
#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif
#include <systemc>
#include "greensocs/libgssync/pre_suspending_sc_support.h"
#include <tlm>
#include <tlm_utils/tlm_quantumkeeper.h>
#include "greensocs/libgssync/async_event.h"
#include "greensocs/libgssync/qk_extendedif.h"
#include "greensocs/libgssync/qkmultithread.h"
#include <greensocs/libgsutils.h>
#include <greensocs/gsutils/uutils.h>

namespace gs {
/* constantly monitor SystemC and dont let it get ahead of the
   local_time - this is the tlm2.0 rule (h) */
void tlm_quantumkeeper_multithread::timehandler() {
    if (!mutex.try_lock())
        return m_tick.notify(sc_core::SC_ZERO_TIME);
    if (status == STOPPED) {
        // NB must be handled from within timehandler SC_METHOD process
        // Otherwise the sc_unsuspend wont be in the right process
        mutex.unlock();
        return sc_core::sc_unsuspend_all();
    }
    if ((get_current_time() > sc_core::sc_time_stamp())) {
        // UnSuspend SystemC if local time is ahead of systemc time
        sc_core::sc_unsuspend_all();
        sc_core::sc_time m_quantum = tlm_utils::tlm_quantumkeeper::get_global_quantum();
        m_tick.notify(std::min(get_current_time() - sc_core::sc_time_stamp(), m_quantum));
    } else {
        // Suspend SystemC if SystemC has caught up with our
        // local_time
        sc_core::sc_suspend_all();
    }

    mutex.unlock();
    cond.notify_all(); // nudge the sync thread, in case it's waiting for us
}

tlm_quantumkeeper_multithread::~tlm_quantumkeeper_multithread() {
    cond.notify_all();
}

// The quantum keeper should be instanced in SystemC
// but it's functions may be called from other threads
// The QK may be instanced outside of elaboration
tlm_quantumkeeper_multithread::tlm_quantumkeeper_multithread()
    : m_systemc_thread_id(std::this_thread::get_id())
    , status(NONE)
    , m_tick(false) /* handle attach manually */ {
    sc_core::sc_spawn_options opt;
    opt.spawn_method();
    opt.set_sensitivity(&m_tick);
    opt.dont_initialize();
    sc_core::sc_spawn(sc_bind(&tlm_quantumkeeper_multithread::timehandler, this), "timehandler",
                      &opt);

    reset();
    SigHandler::get().register_on_exit_cb([this](){stop();});
}

bool tlm_quantumkeeper_multithread::is_sysc_thread() const {
    if (std::this_thread::get_id() == m_systemc_thread_id)
        return true;
    else
        return false;
}

/*
 * Additional functions
 */

void tlm_quantumkeeper_multithread::start(std::function<void()> job) {
    std::lock_guard<std::mutex> lock(mutex);
    status = RUNNING;
    m_tick.async_attach_suspending();
    m_tick.notify(sc_core::SC_ZERO_TIME);
    if (job) {
        std::thread t(job);
        m_worker_thread = std::move(t);
    }
}

void tlm_quantumkeeper_multithread::stop() {
    if (status == RUNNING) {
        std::lock_guard<std::mutex> lock(mutex);
        status = STOPPED;

        m_tick.notify(sc_core::SC_ZERO_TIME);
        cond.notify_all();
        m_tick.async_detach_suspending();

        if (m_worker_thread.joinable()) {
            m_worker_thread.join();
        }
    }
}

/* this may be overloaded to provide a different window */
/* return the time remaining till the next sync point*/
sc_core::sc_time tlm_quantumkeeper_multithread::time_to_sync() {
    if (status != RUNNING)
        return sc_core::SC_ZERO_TIME;
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

void tlm_quantumkeeper_multithread::inc(const sc_core::sc_time& t) {
    std::lock_guard<std::mutex> lock(mutex);
    m_local_time += t;
}

/* NB, if used outside SystemC, SystemC time may vary */
void tlm_quantumkeeper_multithread::set(const sc_core::sc_time& t) {
    std::lock_guard<std::mutex> lock(mutex);
    m_local_time = t + sc_core::sc_time_stamp(); // NB, we store the
    // absolute time.
}

void tlm_quantumkeeper_multithread::sync() {
    if (is_sysc_thread()) {
        assert(m_local_time >= sc_core::sc_time_stamp());
        sc_core::sc_time t = m_local_time - sc_core::sc_time_stamp();
        m_tick.notify();
        sc_core::wait(t);
    } else {
        std::unique_lock<std::mutex> lock(mutex);
        /* Wake up the SystemC thread if it's waiting for us to keep up */
        m_tick.notify();
        /* Wait for some run budget */
        cond.wait(lock,
                  [this] { return status == STOPPED || time_to_sync() != sc_core::SC_ZERO_TIME; });
    }
}

void tlm_quantumkeeper_multithread::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    // As we use absolute time, we reset to the current sc_time
    m_local_time = sc_core::sc_time_stamp();
}

sc_core::sc_time tlm_quantumkeeper_multithread::get_current_time() const {
    return m_local_time;
}

/* NB not thread safe, you're time may vary, you should really be
 * calling this from SystemC */
sc_core::sc_time tlm_quantumkeeper_multithread::get_local_time() const {
    if (m_local_time >= sc_core::sc_time_stamp())
        return m_local_time - sc_core::sc_time_stamp();
    else
        return sc_core::SC_ZERO_TIME;
}

bool tlm_quantumkeeper_multithread::need_sync() {
    // By default we recommend to sync, but other sync policies may vary
    return true;
}
} // namespace gs
