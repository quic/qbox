/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REALTIMLIMITER_H
#define REALTIMLIMITER_H

#include <thread>
#include <mutex>
#include <queue>
#include <future>

#include <systemc>
#include "greensocs/libgssync/async_event.h"

namespace gs {
SC_MODULE (realtimeLimiter) {
    uint RTquantum_ms = 10;

    std::chrono::high_resolution_clock::time_point startRT;
    sc_core::sc_time startSC;
    sc_core::sc_time runto;
    std::thread m_tick_thread;
    bool running = false;

    async_event tick;

    void SCticker()
    {
        if (!running) {
            tick.async_detach_suspending();
            sc_core::sc_unsuspend_all();
            return;
        }

        if (sc_core::sc_time_stamp() >= runto) {
            tick.async_attach_suspending();
            sc_core::sc_suspend_all(); // Dont starve while we're waiting
                                       // for realtime.
        } else {
            // lets go with +=1/2 a RTquantum
            tick.notify((runto + sc_core::sc_time(RTquantum_ms, sc_core::SC_MS) / 2) - sc_core::sc_time_stamp());
            tick.async_detach_suspending(); // We could even starve !
            sc_core::sc_unsuspend_all();
        }
    }

    void RTticker()
    {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(RTquantum_ms));

            runto = sc_core::sc_time(std::chrono::duration_cast<std::chrono::microseconds>(
                                         std::chrono::high_resolution_clock::now() - startRT)
                                         .count(),
                                     sc_core::SC_US) +
                    startSC;
            tick.notify();
        }
    }

public:
    /* NB all these functions should be called from the SystemC thread of course*/
    void enable()
    {
        assert(running == false);

        running = true;

        startRT = std::chrono::high_resolution_clock::now();
        startSC = sc_core::sc_time_stamp();
        runto = sc_core::sc_time(RTquantum_ms, sc_core::SC_MS) + sc_core::sc_time_stamp();
        tick.notify();

        m_tick_thread = std::thread(&realtimeLimiter::RTticker, this);
    }

    void disable()
    {
        if (running) {
            running = false;
            m_tick_thread.join();
        }
    }

    realtimeLimiter(): sc_module(sc_core::sc_module_name("realtimeLimiter")), tick(false) // handle attach manually
    {
        SC_HAS_PROCESS(realtimeLimiter);
        SC_METHOD(SCticker);
        dont_initialize();
        sensitive << tick;
    }
};
} // namespace gs
#endif // REALTIMLIMITER_H
