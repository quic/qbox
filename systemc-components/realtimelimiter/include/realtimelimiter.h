/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REALTIMLIMITER_H
#define REALTIMLIMITER_H

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <thread>
#include <mutex>
#include <queue>
#include <future>

#include <systemc>
#include <cci_configuration>
#include <scp/report.h>

#include <async_event.h>
#include <module_factory_registery.h>

namespace gs {
/**
 * @brief realtimelimiter: sc_module which suspends SystemC if SystemC time drifts ahead of realtime
 * @param @RTquantum_ms : realtime tick rate between checks.
 * @param @SCTimeout_ms : If SystemC time is behind by more than this value, then generate a fatal abort (0 disables)
 *
 */
SC_MODULE (realtimelimiter) {
    SCP_LOGGER();
    cci::cci_param<double> p_RTquantum_ms;
    cci::cci_param<double> p_SCTimeout_ms;
    cci::cci_param<double> p_MaxTime_ms;

    std::chrono::high_resolution_clock::time_point startRT;
    sc_core::sc_time startSC;
    sc_core::sc_time runto;
    std::thread m_tick_thread;
    bool running = false;
    bool suspended = false;
    sc_core::sc_time suspend_at = sc_core::SC_ZERO_TIME;
    async_event tick;

    void SCticker()
    {
        if (!running) {
            tick.async_detach_suspending();
            sc_core::sc_unsuspend_all();
            return;
        }
        if (suspended == true && (sc_core::sc_time_stamp() > suspend_at)) {
            /* If we were suspended, and time has drifted, then account for it here. */
            sc_core::sc_time drift = sc_core::sc_time_stamp() - suspend_at;
            startSC += drift;
            SCP_WARN(())("Time drift of {}s", drift.to_seconds());
        }
        if (p_MaxTime_ms && sc_core::sc_time_stamp() > (sc_core::sc_time(p_MaxTime_ms, sc_core::SC_MS) + startSC)) {
            SCP_FATAL(())("Max timeout expired");
        }

        if (sc_core::sc_time_stamp() >= runto) {
            SCP_TRACE(())("Suspending");
            tick.async_attach_suspending();
            sc_core::sc_suspend_all(); // Dont starve while we're waiting
                                       // for realtime.
            suspend_at = sc_core::sc_time_stamp();
            suspended = true;
        } else {
            SCP_TRACE(())("Resuming");
            suspended = false;
            // lets go with +=1/2 a RTquantum
            tick.notify((runto + sc_core::sc_time(p_RTquantum_ms, sc_core::SC_MS) / 2) - sc_core::sc_time_stamp());
            tick.async_detach_suspending(); // We could even starve !
            sc_core::sc_unsuspend_all();
        }
    }

    void RTticker()
    {
        sc_core::sc_time last = sc_core::SC_ZERO_TIME;
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(p_RTquantum_ms));

            runto = sc_core::sc_time(std::chrono::duration_cast<std::chrono::microseconds>(
                                         std::chrono::high_resolution_clock::now() - startRT)
                                         .count(),
                                     sc_core::SC_US) +
                    startSC;
            if (last > sc_core::SC_ZERO_TIME && sc_core::sc_time_stamp() == last) {
                p_RTquantum_ms.set_value(p_RTquantum_ms.get_value() + 100);
                if (p_SCTimeout_ms) {
                    SCP_WARN(())
                    ("Stalled ? (runto is {}s ahead, systemc time has not changed, new limit {}ms {}s)",
                     (runto - sc_core::sc_time_stamp()).to_seconds(), p_RTquantum_ms.get_value(), runto.to_seconds());
                }
                /* Only check for exsessive runto's if we're stalled */
                if (p_SCTimeout_ms &&
                    (runto > sc_core::sc_time_stamp() + sc_core::sc_time(p_SCTimeout_ms, sc_core::SC_MS))) {
                    SCP_FATAL(())
                    ("Requested runto is {}s ahead (SCTimeout_ms set to {}s)",
                     (runto - sc_core::sc_time_stamp()).to_seconds(), p_SCTimeout_ms / 1000);
                }
            } else {
                last = sc_core::sc_time_stamp();
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
        runto = sc_core::sc_time(p_RTquantum_ms, sc_core::SC_MS) + sc_core::sc_time_stamp();
        tick.notify(sc_core::sc_time(p_RTquantum_ms, sc_core::SC_MS));

        m_tick_thread = std::thread(&realtimelimiter::RTticker, this);
    }

    void disable()
    {
        if (running) {
            running = false;
            m_tick_thread.join();
        }
    }
    realtimelimiter(const sc_core::sc_module_name& name): realtimelimiter(name, true) {}
    realtimelimiter(const sc_core::sc_module_name& name, bool autostart)
        : sc_module(name)
        , p_RTquantum_ms("RTquantum_ms", 100, "Real time quantum in milliseconds")
        , p_SCTimeout_ms("SCTimeout_ms", 0, "Timeout for SystemC in milliseconds")
        , p_MaxTime_ms("MaxTime_ms", 0, "Maximum run time in ms (0=no limit)")
        , tick(false) // handle attach manually
    {
        SCP_TRACE(())("realtimelimiter constructor");
        SC_HAS_PROCESS(realtimelimiter);
        SC_METHOD(SCticker);
        dont_initialize();
        sensitive << tick;
        if (autostart) {
            SC_THREAD(enable);
        }
    }

    void end_of_simulation() { disable(); }
};
} // namespace gs

extern "C" void module_register();

#endif // REALTIMLIMITER_H
