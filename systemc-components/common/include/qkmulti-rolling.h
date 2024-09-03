/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QKMULTI_ROLLING_H
#define QKMULTI_ROLLING_H

#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif
#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <scp/report.h>

#include <qkmulti-quantum.h>
#include <semaphore.h>
#include <libgsutils.h>

namespace gs {
class tlm_quantumkeeper_multi_rolling : public tlm_quantumkeeper_multi_quantum
{
private:
    async_event m_time_ev;
    sc_core::sc_time m_next_time;
    semaphore m_sem;

    sc_core::sc_time get_next_time()
    {
        if (status != RUNNING) return sc_core::SC_ZERO_TIME;

        sc_core::sc_time quantum = tlm_utils::tlm_quantumkeeper::get_global_quantum();
        sc_core::sc_time next_event = sc_core::sc_time_to_pending_activity();
        sc_core::sc_time quantum_boundary = sc_core::sc_time_stamp() + quantum - get_current_time();
        if (sc_core::sc_pending_activity_at_current_time()) {
            SCP_INFO("Libgssync") << "Pending activity now returning SC_ZERO_TIME";
            m_tick.notify();
            return sc_core::SC_ZERO_TIME;
        } else if (sc_core::sc_pending_activity_at_future_time()) {
            sc_core::sc_time ret = std::min(next_event, quantum_boundary);
            SCP_INFO("Libgssync") << "Pending activity returning " << ret.to_string();
            return ret;
        } else {
            SCP_INFO("Libgssync") << "No pending activity returning quantum boundary " << quantum_boundary.to_string();
            return quantum_boundary;
        }
    }

    void get_time_from_systemc()
    {
        m_next_time = get_next_time();
        m_sem.notify();
    }

    virtual sc_core::sc_time time_to_sync() override
    {
        if (is_sysc_thread())
            return get_next_time();
        else {
            m_time_ev.notify();
            m_sem.wait();
            return m_next_time;
        }
    }

public:
    tlm_quantumkeeper_multi_rolling(): m_time_ev(false)
    {
        sc_core::sc_spawn_options opt;
        opt.spawn_method();
        opt.set_sensitivity(&m_time_ev);
        opt.dont_initialize();
        sc_core::sc_spawn(sc_bind(&tlm_quantumkeeper_multi_rolling::get_time_from_systemc, this), "get_time_from_sysc",
                          &opt);
    }
};
} // namespace gs
#endif // QKMULTI_ROLLING_H
