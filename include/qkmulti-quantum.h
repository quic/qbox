/*
 *  Copyright (C) 2020  GreenSocs
 *
 */

#pragma once

#include "qkmultithread.h"

namespace gs {
    class tlm_quantumkeeper_multi_quantum : public tlm_quantumkeeper_multithread {
        // Only allow up to one quantum from the current sc_time
        // In accordance with TLM-2
        virtual sc_core::sc_time time_to_sync() override {
            if (status != RUNNING)
                return sc_core::SC_ZERO_TIME;

            sc_core::sc_time quantum = tlm_utils::tlm_quantumkeeper::get_global_quantum();
            sc_core::sc_time next_quantum_boundary = sc_core::sc_time_stamp() + quantum;
            sc_core::sc_time now = get_current_time();
            if (next_quantum_boundary >= now) {
                return next_quantum_boundary - now;
            } else {
                return sc_core::SC_ZERO_TIME;
            }
        }

        // Don't sync until we are on the quantum boundry (as expected by TLM-2)
        virtual bool need_sync() override {
            return time_to_sync() == sc_core::SC_ZERO_TIME;
        }
    };
}
