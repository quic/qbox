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

#ifndef QKMULTI_MULTI_QUANTUM_H
#define QKMULTI_MULTI_QUANTUM_H

#include "greensocs/libgssync/qkmultithread.h"

namespace gs {
class tlm_quantumkeeper_multi_quantum : public tlm_quantumkeeper_multithread
{
    // Only allow up to one quantum from the current sc_time
    // In accordance with TLM-2
    virtual sc_core::sc_time time_to_sync() override
    {
        if (status != RUNNING) return sc_core::SC_ZERO_TIME;

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
    virtual bool need_sync() override { return time_to_sync() == sc_core::SC_ZERO_TIME; }
};
} // namespace gs
#endif