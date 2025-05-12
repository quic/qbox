/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <tlm>
#include <cci_configuration>
#include <scp/report.h>
#include <mutex>

#ifdef SC_HAS_OB_EVENT
#include <sysc/utils/sc_ob_event.h>
#else
#include "observer_event.h"
typedef gs::observer_event sc_ob_event;
#endif

namespace sc_core {

/**
 * @brief Base class which gives the quantum (dynamically) and specifies if the
 * window should stay open indefinitely or detach when there are no more events.
 * A SC_ZERO_TIME quantum means sync on every time step
 */
struct sync_policy_base {
    virtual sc_core::sc_time quantum() = 0;
    virtual bool keep_alive() = 0;
};
struct sync_policy_sc_zero_time : public sync_policy_base {
    sc_core::sc_time quantum() { return SC_ZERO_TIME; }
    bool keep_alive() { return false; }
};
struct sync_policy_tlm_quantum : public sync_policy_base {
    sc_core::sc_time quantum() { return tlm_utils::tlm_quantumkeeper::get_global_quantum(); }
    bool keep_alive() { return true; }
};

/**
 * @brief windowed synchronizer, template SYNC_POLICY gives the quantum (dynamically) and specifies if the
 * window should stay open indefinitely or detach when there are no more events.
 *
 * @tparam SYNC_POLICY
 */
template <class SYNC_POLICY = sync_policy_sc_zero_time>
class sync_window : public sc_core::sc_module, public sc_core::sc_prim_channel
{
    static_assert(std::is_base_of<sync_policy_base, SYNC_POLICY>::value,
                  "SYNC_POLICY must derive from sync_policy_base");

    sc_core::sc_event m_sweep_ev;
    sc_ob_event m_step_ev;
    std::mutex m_mutex;
    SYNC_POLICY policy;

public:
    struct window {
        sc_core::sc_time from;
        sc_core::sc_time to;
        bool operator==(window other) { return other.to == to && other.from == from; }
    };

    const struct window zero_window = { sc_core::SC_ZERO_TIME, sc_core::SC_ZERO_TIME };
    const struct window open_window = { sc_core::SC_ZERO_TIME, sc_core::sc_max_time() };

private:
    window m_window;
    std::function<void(const window&)> m_other_async_set_window_fn;

    void do_other_async_set_window_fn(const window& p_w)
    {
        if (m_other_async_set_window_fn) {
            m_other_async_set_window_fn(p_w);
        }
    }

    /* Handle suspending/resuming at the end of the window, also inform other side if we reach end of window */
    void step_helper()
    {
        auto now = sc_core::sc_time_stamp();
        auto to = m_window.to;
        SCP_INFO(()) << "End window check"
                     << " their window " << m_window.from << " " << to;

        if (now >= to) {
            sc_core::sc_unsuspend_all(); // such that pending activity is valid if it's needed below.

            /* We should suspend at this point, and wait for the other side to catch up */
            auto q = (policy.quantum() == sc_core::SC_ZERO_TIME) ? sc_core::sc_time_to_pending_activity()
                                                                 : policy.quantum();
            SCP_TRACE(()) << "Suspend at: " << now << " quantum: " << q;
            do_other_async_set_window_fn({ now, now + q });

            if (!policy.keep_alive()) async_attach_suspending();
            sc_core::sc_suspend_all();

        } else {
            sc_core::sc_unsuspend_all();
            if (!policy.keep_alive()) async_detach_suspending();

            SCP_TRACE(()) << "Resume, Step till: " << to;

            auto q = (policy.quantum() == sc_core::SC_ZERO_TIME) ? sc_core::sc_time_to_pending_activity()
                                                                 : policy.quantum();
            do_other_async_set_window_fn({ now, now + q });
            /* Re-notify event - maybe presumably moved */
            m_step_ev.notify(std::min(q, to - now));
        }
    }

    /*
     * Handle all sweep requests, once we arrive at a sweep point, tell the other side.
     */
    void sweep_helper()
    {
        auto now = sc_core::sc_time_stamp();

        SCP_INFO(()) << "Done sweep"
                     << " their window " << m_window.from << " - " << m_window.to;

        auto q = (policy.quantum() == sc_core::SC_ZERO_TIME) ? sc_core::sc_time_to_pending_activity()
                                                             : policy.quantum();
        do_other_async_set_window_fn({ now, now + q });
    }

    /* Manage the Sync aync update  */
    void update()
    {
        std::lock_guard<std::mutex> lg(m_mutex);
        auto now = sc_core::sc_time_stamp();
        SCP_INFO(()) << "Update"
                     << " their window " << m_window.from << " - " << m_window.to;

        if (m_window.from > now) {
            m_sweep_ev.notify(m_window.from - now); // Try to move time forward.
        } else {
            m_sweep_ev.cancel(); // no need to fire event.
        }

        /* let stepper handle suspend/resume */
        m_step_ev.notify(sc_core::SC_ZERO_TIME);
        if (m_window == open_window) {
            /* The other side is signalling that they have no more events */
            SCP_DEBUG(())("Stopping");
            sc_stop();
        }
    }

public:
    SCP_LOGGER();

    /* API call from other pair, presumably in a different thread.
     * The internal window wil be updated atomically.
     *
     * Input: window  - Window to set for sync. Sweep till the 'from' and step to the 'to'.
     */
    void async_set_window(window w)
    {
        /* Only accept updated windows so we dont re-send redundant updates */
        std::lock_guard<std::mutex> lg(m_mutex);
        if (!(w == m_window)) {
            SCP_INFO(()) << "sync " << w.from << " " << w.to;
            m_window = w;
            async_request_update();
        }
    }

    void bind(sync_window* other)
    {
        if (m_other_async_set_window_fn) {
            SCP_WARN(())
            ("m_other_async_set_window_fn was already registered or other sync_window was already bound!");
        }
        m_other_async_set_window_fn = [other](const window& w) { other->async_set_window(w); };
    }
    void register_sync_cb(std::function<void(const window&)> fn)
    {
        if (m_other_async_set_window_fn) {
            SCP_WARN(())
            ("m_other_async_set_window_fn was already registered or other sync_window was already bound!");
        }
        m_other_async_set_window_fn = fn;
    }
    SC_CTOR (sync_window) : m_window({sc_core::SC_ZERO_TIME, policy.quantum()})
        {
            SCP_TRACE(())("Constructor");
            SC_METHOD(sweep_helper);
            dont_initialize();
            sensitive << m_sweep_ev;

            SC_METHOD(step_helper);
            // Do initialize
            sensitive << m_step_ev;

            this->sc_core::sc_prim_channel::async_attach_suspending();
        }
};

} // namespace sc_core