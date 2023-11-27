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
#include <cci_configuration>
#include <scp/report.h>

#include "greensocs/libgssync/async_event.h"

namespace gs {
/**
 * @brief realtimeLimiter: sc_module which suspends SystemC if SystemC time drifts ahead of realtime
 * @param @RTquantum_ms : realtime tick rate between checks.
 * @param @SCTimeout_ms : If SystemC time is behind by more than this value, then generate a fatal abort (0 disables)
 *
 */
SC_MODULE (realtimeLimiter) {
    SCP_LOGGER();
    cci::cci_param<double> RTquantum_ms;
    cci::cci_param<double> SCTimeout_ms;

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
        sc_core::sc_time last = sc_core::SC_ZERO_TIME;
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(RTquantum_ms));

            runto = sc_core::sc_time(std::chrono::duration_cast<std::chrono::microseconds>(
                                         std::chrono::high_resolution_clock::now() - startRT)
                                         .count(),
                                     sc_core::SC_US) +
                    startSC;
            if (last > sc_core::SC_ZERO_TIME && sc_core::sc_time_stamp() == last) {
                SCP_WARN(())
                ("Stalled ? (runto is {}s ahead, systemc time has not changed)",
                 (runto - sc_core::sc_time_stamp()).to_seconds());
            } else {
                last = sc_core::sc_time_stamp();
            }
            if (SCTimeout_ms && (runto > sc_core::sc_time_stamp() + sc_core::sc_time(SCTimeout_ms, sc_core::SC_MS))) {
                SCP_FATAL(())
                ("Requested runto is {}s ahead (SCTimeout_ms set to {}s)",
                 (runto - sc_core::sc_time_stamp()).to_seconds(), SCTimeout_ms / 1000);
            }

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
        tick.notify(sc_core::sc_time(RTquantum_ms, sc_core::SC_MS));

        m_tick_thread = std::thread(&realtimeLimiter::RTticker, this);
    }

    void disable()
    {
        if (running) {
            running = false;
            m_tick_thread.join();
        }
    }
    realtimeLimiter(const sc_core::sc_module_name& name): realtimeLimiter(name, true) {}
    realtimeLimiter(const sc_core::sc_module_name& name, bool autostart)
        : sc_module(name)
        , RTquantum_ms("RTquantum_ms", 100, "Real time quantum in milliseconds")
        , SCTimeout_ms("SCTimeout_ms", 0, "Timeout for SystemC in milliseconds")
        , tick(false) // handle attach manually
    {
        SC_HAS_PROCESS(realtimeLimiter);
        SC_METHOD(SCticker);
        SCP_TRACE(())("realtimelimiter constructor");
        dont_initialize();
        sensitive << tick;
        if (autostart) {
            SC_THREAD(enable);
        }
    }

    void end_of_simulation() { disable(); }
};
} // namespace gs
typedef gs::realtimeLimiter realtimeLimiter;
GSC_MODULE_REGISTER(realtimeLimiter);

#endif // REALTIMLIMITER_H
