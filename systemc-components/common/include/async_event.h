/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ASYNC_EVENT
#define ASYNC_EVENT

#include <iostream>

#include <systemc>
#include <tlm>
#include <thread>
#include <mutex>

#include <pre_suspending_sc_support.h>

namespace gs {
#if SYSTEMC_VERSION < 20231124
class async_event : public sc_core::sc_prim_channel, public sc_core::sc_event
#else
class async_event : public sc_core::sc_prim_channel, public sc_core::sc_event, public sc_core::sc_stage_callback_if
#endif
{
private:
    sc_core::sc_time m_delay;
    std::thread::id tid;
    std::mutex mutex; // Belt and braces
    bool outstanding;

public:
    async_event(bool start_attached = true): outstanding(0)
    {
#if SYSTEMC_VERSION < 20231124
        register_simulation_phase_callback(sc_core::SC_PAUSED | sc_core::SC_START_OF_SIMULATION);
#else
        sc_core::sc_register_stage_callback(*this, sc_core::SC_START_OF_SIMULATION);
#endif
        tid = std::this_thread::get_id();
        outstanding = false;
        enable_attach_suspending(start_attached);
    }

    void async_notify() { notify(); }

    void notify(sc_core::sc_time delay = sc_core::sc_time(sc_core::SC_ZERO_TIME))
    {
        if (tid == std::this_thread::get_id()) {
            sc_core::sc_event::notify(delay);
        } else {
            mutex.lock();
            m_delay = delay;
            outstanding = true;
            async_request_update();
            mutex.unlock();
#ifndef SC_HAS_SUSPENDING
            sc_core::sc_internal_async_wakeup();
#endif
        }
    }

    void async_attach_suspending()
    {
#ifndef SC_HAS_ASYNC_ATTACH_SUSPENDING
        sc_core::async_attach_suspending(this);
#else
        this->sc_core::sc_prim_channel::async_attach_suspending();
#endif
    }

    void async_detach_suspending()
    {
#ifndef SC_HAS_ASYNC_ATTACH_SUSPENDING
        sc_core::async_detach_suspending(this);
#else
        this->sc_core::sc_prim_channel::async_detach_suspending();
#endif
    }

    void enable_attach_suspending(bool e)
    {
        e ? this->async_attach_suspending() : this->async_detach_suspending();
    }

private:
    void update(void)
    {
        mutex.lock();
        // we should be in SystemC thread
        if (outstanding) {
            sc_event::notify(m_delay);
            outstanding = false;
        }
        mutex.unlock();
    }
#if SYSTEMC_VERSION >= 20231124
    void stage_callback(const sc_core::sc_stage& stage) { simulation_phase_callback(); }
#endif
    void simulation_phase_callback()
    {
        // we should be in SystemC thread
        if (outstanding) {
            request_update();
        }
    }
};
} // namespace gs

#endif // ASYNC_EVENT
