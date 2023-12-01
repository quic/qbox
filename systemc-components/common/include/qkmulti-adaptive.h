/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QKMULTI_ADAPTIVE_QUANTUM_H
#define QKMULTI_ADAPTIVE_QUANTUM_H

#include <qkmultithread.h>

namespace gs {
class tlm_quantumkeeper_multi_adaptive : public tlm_quantumkeeper_multithread
{
    // Only allow up to one quantum from the current sc_time
    // In accordance with TLM-2
    virtual sc_core::sc_time time_to_sync() override
    {
        if (status != RUNNING) return sc_core::SC_ZERO_TIME;

        sc_core::sc_time m_quantum = tlm_utils::tlm_quantumkeeper::get_global_quantum();
        sc_core::sc_time sct = sc_core::sc_time_stamp();
        sc_core::sc_time rmt = get_current_time();
        if (rmt <= sct) {
            return m_quantum * 2;
        }
        if (rmt > sct && rmt <= sct + m_quantum) {
            return m_quantum * 1;
        }
        if (rmt > sct + m_quantum && rmt <= sct + (2 * m_quantum)) {
            return m_quantum * 0.5;
        }
        return sc_core::SC_ZERO_TIME;
    }

    // Don't sync until we are on the quantum boundry (as expected by TLM-2)
    virtual bool need_sync() override { return time_to_sync() == sc_core::SC_ZERO_TIME; }
};
} // namespace gs
#endif