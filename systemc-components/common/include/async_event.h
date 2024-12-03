/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ASYNC_EVENT
#define ASYNC_EVENT

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <iostream>

#include <systemc>
#include <tlm>
#include <thread>
#include <mutex>

#include <pre_suspending_sc_support.h>

namespace gs {
class async_event : public sc_core::sc_prim_channel, public sc_core::sc_event
{
private:
    sc_core::sc_time m_delay;
    std::thread::id tid;
    std::mutex mutex; // Belt and braces
    bool outstanding = false;
    enum { none, attach, detach } attach_pending_state = none;

public:
    async_event(bool start_attached = true): outstanding(0)
    {
        tid = std::this_thread::get_id();
        enable_attach_suspending(start_attached);
    }

    void async_notify() { notify(); }

    void notify()
    {
        if (tid == std::this_thread::get_id()) {
            sc_core::sc_event::notify();
        } else {
            notify(sc_core::SC_ZERO_TIME);
        }
    }
    void notify(double d, sc_core::sc_time_unit u) { notify(sc_core::sc_time(d, u)); }
    void notify(sc_core::sc_time delay)
    {
        if (tid == std::this_thread::get_id()) {
            sc_core::sc_event::notify(delay);
        } else {
            mutex.lock();
            m_delay = delay;
            outstanding = true;
            mutex.unlock();
            async_request_update();
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
        if (tid == std::this_thread::get_id()) {
            this->sc_core::sc_prim_channel::async_attach_suspending();
        } else {
            mutex.lock();
            attach_pending_state = attach;
            mutex.unlock();
            async_request_update();
        }
#endif
    }

    void async_detach_suspending()
    {
#ifndef SC_HAS_ASYNC_ATTACH_SUSPENDING
        sc_core::async_detach_suspending(this);
#else
        if (tid == std::this_thread::get_id()) {
            this->sc_core::sc_prim_channel::async_detach_suspending();
        } else {
            mutex.lock();
            attach_pending_state = detach;
            mutex.unlock();
            async_request_update();
        }
#endif
    }

    void enable_attach_suspending(bool e) { e ? this->async_attach_suspending() : this->async_detach_suspending(); }

private:
    void update(void)
    {
        mutex.lock();
        // we should be in SystemC thread
        if (outstanding) {
            sc_event::notify(m_delay);
            outstanding = false;
        }
        switch (attach_pending_state) {
        case attach:
            this->async_attach_suspending();
            break;
        case detach:
            this->async_detach_suspending();
            break;
        default:
            break;
        }
        attach_pending_state = none;
        mutex.unlock();
    }
    void start_of_simulation()
    {
        // we should be in SystemC thread
        if (outstanding) {
            request_update();
        }
    }
};
} // namespace gs

#endif // ASYNC_EVENT
