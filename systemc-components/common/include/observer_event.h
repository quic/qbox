/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GS_OBSERVER_EVENT_
#define _GS_OBSERVER_EVENT_

#include <systemc>
#include <tlm>
#include <scp/report.h>
#include <string>

namespace gs {
class observer_event : public sc_core::sc_module, public sc_core::sc_event, public sc_core::sc_stage_callback_if
{
    SCP_LOGGER();

private:
    sc_core::sc_event m_update;
    sc_core::sc_time m_scheduled;
    sc_core::sc_process_handle m_th;

    void update_m()
    {
        auto now = sc_core::sc_time_stamp();
        if (now >= m_scheduled) {
            sc_core::sc_event::notify();
        } else {
            sc_core::sc_time pending = now;
            if (sc_core::sc_pending_activity_at_future_time()) {
                pending = now + sc_core::sc_time_to_pending_activity();
            }
            SCP_TRACE(()) << "time of future pending activity: " << pending.to_seconds() << " sec";
            if (pending >= m_scheduled) {
                sc_core::sc_event::notify(m_scheduled - now);
            } else {
                if (pending > now) {
                    m_update.notify(pending - now);
                } else {
                    sc_core::sc_register_stage_callback(*this, sc_core::SC_POST_UPDATE);
                }
            }
        }
    }

    void future_events_notify_th()
    {
        while (true) {
            m_th.suspend();
            update_m();
        }
    }

public:
    observer_event(const sc_core::sc_module_name& n = sc_core::sc_gen_unique_name("observer_event"))
        : sc_module(n)
        , sc_core::sc_event((std::string(n) + "_notifier_event").c_str())
        , m_update((std::string(n) + "_update_event").c_str())
        , m_scheduled(sc_core::SC_ZERO_TIME)
    {
        SC_METHOD(update_m);
        sensitive << m_update;
        dont_initialize();
        SC_THREAD(future_events_notify_th);
        m_th = sc_core::sc_get_current_process_handle();
    }
    void notify()
    {
        m_scheduled = sc_core::sc_time_stamp();
        m_update.notify(sc_core::SC_ZERO_TIME);
    }
    void notify(double delay, sc_core::sc_time_unit unit)
    {
        m_scheduled = sc_core::sc_time_stamp() + sc_core::sc_time(delay, unit);
        m_update.notify(sc_core::SC_ZERO_TIME);
    }
    void notify(sc_core::sc_time delay)
    {
        m_scheduled = sc_core::sc_time_stamp() + delay;
        m_update.notify(sc_core::SC_ZERO_TIME);
    }

    void stage_callback(const sc_core::sc_stage& stage)
    {
        if (sc_core::sc_pending_activity_at_future_time()) {
            m_th.resume();
            sc_core::sc_unregister_stage_callback(*this, sc_core::SC_POST_UPDATE);
        }
    }

    ~observer_event() {}
};
} // namespace gs

#endif // OBSERVER_EVENT