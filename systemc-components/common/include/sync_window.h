/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: Mark Burton 2025
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <cci_configuration>
#include <scp/report.h>

#include "observer_event.h"

namespace gs {

class sync_windowed : public sc_core::sc_module, public sc_core::sc_prim_channel
{
    cci::cci_param<sc_core::sc_time> p_quantum;

    sc_event m_sweep_ev;
    observer_event m_step_ev;

public:
    struct window {
        sc_core::sc_time from;
        sc_core::sc_time to;
    };
    const struct window zero_window = { sc_core::SC_ZERO_TIME, sc_core::SC_ZERO_TIME };
    const struct window open_window = { sc_core::SC_ZERO_TIME, sc_core::sc_max_time() };

private:
    std::atomic<struct window> m_window = zero_window;

    // sync_windowed* m_other;
    std::function<void(struct window)> m_other_sync_fn;

    /* Handle suspending/resuming at the end of the window, also inform other side if we reach end of window */
    void step_helper()
    {
        auto now = sc_core::sc_time_stamp();
        auto window = m_window.load();
        auto to = window.to;
        SCP_INFO(()) << "End window check"
                     << " their window " << window.from << " " << to;

        if (now >= to) {
            sc_core::sc_unsuspend_all(); // such that pending activity is valid if it's needed below.

            /* We should suspend at this point, and wait for the other side to catch up */
            auto q = (p_quantum.get_value() == sc_core::SC_ZERO_TIME) ? sc_core::sc_time_to_pending_activity()
                                                                      : p_quantum;
            SCP_TRACE(()) << "Suspend : window till " << now + q;
            m_other_sync_fn({ now, now + q });

            async_attach_suspending();
            sc_core::sc_suspend_all();

        } else {
            SCP_TRACE(()) << "Resume";

            sc_core::sc_unsuspend_all();
            async_detach_suspending();

            SCP_TRACE(()) << "Step till " << to;

            auto q = (p_quantum.get_value() == sc_core::SC_ZERO_TIME) ? sc_core::sc_time_to_pending_activity()
                                                                      : p_quantum;
            m_other_sync_fn({ now, now + q });
            /* Re-notify event - maybe presumably moved */
            m_step_ev.notify(to - now);
        }
    }

    /*
     * Handle all sweep requests, once we arrive at a sweep point, tell the other side.
     */
    virtual void sweep_helper()
    {
        auto now = sc_core::sc_time_stamp();

        auto window = m_window.load();
        SCP_INFO(()) << "Done sweep"
                     << " their window " << window.from << " - " << window.to;

        auto q = (p_quantum.get_value() == sc_core::SC_ZERO_TIME) ? sc_core::sc_time_to_pending_activity() : p_quantum;
        m_other_sync_fn({ now, now + q });
    }

    /* Manage the Sync aync update  */
    void update()
    {
        auto now = sc_core::sc_time_stamp();
        auto window = m_window.load();
        SCP_INFO(()) << "Update"
                     << " their window " << window.from << " - " << window.to;

        if (window.from > now) {
            m_sweep_ev.notify(window.from - now); // Try to move time forward.
        } else {
            m_sweep_ev.cancel(); // no need to fire event.
        }

        /* let stepper handle suspend/resume */
        m_step_ev.notify(sc_core::SC_ZERO_TIME);
    }

public:
    SCP_LOGGER();

    /* API call from other pair, presumably in a different thread.
     * The internal window wil be updated atomically.
     *
     * Input: window  - Window to set for sync. Sweep till the 'from' and step to the 'to'.
     */
    void sync(window w)
    {
        auto m_w = m_window.load();
        /* Only accept updated windows so we dont re-send redundant updates */
        if ((w.to > m_w.to) || (w.from > m_w.from)) {
            SCP_INFO(()) << "sync " << w.from << " " << w.to;
            m_window = w;
            async_request_update();
        }
    }

    void end_of_simulation() { m_other_sync_fn(open_window); }

    void start_of_simulation()
    {
        SCP_WARN(())("Start of sim");
        m_step_ev.notify(sc_core::SC_ZERO_TIME);
    }
    void bind(sync_windowed* other)
    {
        m_other_sync_fn = [other](struct window w) { other->sync(w); };
    }
    void register_sync_cb(std::function<void(struct window)> fn) { m_other_sync_fn = fn; }

    SC_CTOR (sync_windowed, sc_core::sc_time q = sc_core::SC_ZERO_TIME):
         p_quantum("Quantum", q, "Quantum to use, SC_ZERO_TIME (The default) means sync on every time step")
        {
            SCP_TRACE(())("Constructor");
            SC_METHOD(sweep_helper);
            dont_initialize();
            sensitive << m_sweep_ev;

            SC_METHOD(step_helper);
            dont_initialize();
            sensitive << m_step_ev;
        }
};

} // namespace gs