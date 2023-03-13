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

#ifndef QK_EXTENDEDIF_H
#define QK_EXTENDEDIF_H

#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif
#include <systemc>
#include <tlm>
#include <tlm_utils/tlm_quantumkeeper.h>

#include <scp/report.h>

#include <greensocs/libgsutils.h>

namespace gs {

/* this quantumkeeper is inteded as an interface for other more
 * sophisticated QKs. However if provides a trivial implementation making it
 * identicle to a standard tlm-2.0 QK
 */
namespace SyncPolicy {
enum Type { SYSTEMC_THREAD, OS_THREAD };
}

class tlm_quantumkeeper_extended : public tlm_utils::tlm_quantumkeeper
{
public:
    tlm_quantumkeeper_extended() {}

    /*
     * Additional functions
     */

    // based on standard tlm2 QK.
    virtual sc_core::sc_time time_to_sync() {
        if (m_next_sync_point > (m_local_time + sc_core::sc_time_stamp()))
            return m_next_sync_point - (m_local_time + sc_core::sc_time_stamp());
        else
            return sc_core::SC_ZERO_TIME;
    }

    // stop trying to syncronise
    virtual void stop() {}

    // start trying to syncronise, (and start job for convenience)
    virtual void start(std::function<void()> job = nullptr) {
        if (job)
            sc_core::sc_spawn(job);
    }

    virtual SyncPolicy::Type get_thread_type() const { return SyncPolicy::SYSTEMC_THREAD; }

    // non-const need_sync as some sync policies need a non-const version
    virtual bool need_sync() { return tlm_utils::tlm_quantumkeeper::need_sync(); }

    virtual bool need_sync() const override {
        SCP_INFO("Libgssync")
            << "const need_sync called, probably wanted to call non-const version";
        return tlm_utils::tlm_quantumkeeper::need_sync();
    }

    // NB, the following function is only for the convenience of
    // the 'old' sync policys. This can/should be removed.
    virtual void run_on_systemc(std::function<void()> job) { job(); }
};
} // namespace gs

#endif // QK_EXTENDEDIF_H
