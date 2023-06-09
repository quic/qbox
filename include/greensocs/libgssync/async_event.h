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

#ifndef ASYNC_EVENT
#define ASYNC_EVENT

#include <iostream>

#include <systemc>
#include <tlm>
#include <thread>
#include <mutex>

#include "greensocs/libgssync/pre_suspending_sc_support.h"

namespace gs {
class async_event : public sc_core::sc_prim_channel, public sc_core::sc_event
{
private:
    sc_core::sc_time m_delay;
    std::thread::id tid;
    std::mutex mutex; // Belt and braces
    int outstanding;

public:
    async_event(bool start_attached = true): outstanding(0) {
        register_simulation_phase_callback(sc_core::SC_PAUSED | sc_core::SC_START_OF_SIMULATION);
        tid = std::this_thread::get_id();
        outstanding = 0;
        enable_attach_suspending(start_attached);
    }

    void async_notify() { notify(); }

    void notify(sc_core::sc_time delay = sc_core::sc_time(sc_core::SC_ZERO_TIME)) {
        if (tid == std::this_thread::get_id()) {
            sc_core::sc_event::notify(delay);
        } else {
            mutex.lock();
            m_delay = delay;
            outstanding++;
            mutex.unlock();
            async_request_update();
#ifndef SC_HAS_SUSPENDING
            sc_core::sc_internal_async_wakeup();
#endif
        }
    }

    void async_attach_suspending() {
#ifndef SC_HAS_ASYNC_ATTACH_SUSPENDING
        sc_core::async_attach_suspending(this);
#else
        this->sc_core::sc_prim_channel::async_attach_suspending();
#endif
    }

    void async_detach_suspending() {
#ifndef SC_HAS_ASYNC_ATTACH_SUSPENDING
        sc_core::async_detach_suspending(this);
#else
        this->sc_core::sc_prim_channel::async_detach_suspending();
#endif
    }

    void enable_attach_suspending(bool e) {
        mutex.lock();
        e ? this->async_attach_suspending() : this->async_detach_suspending();
        mutex.unlock();
    }

private:
    void update(void) {
        mutex.lock();
        // we should be in SystemC thread
        if (outstanding) {
            sc_event::notify(m_delay);
            outstanding--;
            if (outstanding)
                request_update();
        }
        mutex.unlock();
    }

    void simulation_phase_callback() {
        mutex.lock();
        if (outstanding)
            request_update();
        mutex.unlock();
    }
};
} // namespace gs

#endif // ASYNC_EVENT
