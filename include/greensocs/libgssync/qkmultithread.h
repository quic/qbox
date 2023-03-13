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

#ifndef QKMULTITHREAD_H
#define QKMULTITHREAD_H

#include <mutex>
#include <condition_variable>
#include <systemc>
#include <tlm>
#include <tlm_utils/tlm_quantumkeeper.h>
#include "async_event.h"
#include "qk_extendedif.h"

namespace gs {
// somewhat tuned multiple threaded QK
class tlm_quantumkeeper_multithread : public gs::tlm_quantumkeeper_extended
{
    std::thread::id m_systemc_thread_id;
    std::mutex mutex;
    std::condition_variable cond;
    std::thread m_worker_thread;

protected:
    enum jobstates { NONE, RUNNING, STOPPED } status;
    async_event m_tick;

    virtual bool is_sysc_thread() const;

private:
    void timehandler();

public:
    virtual ~tlm_quantumkeeper_multithread();
    tlm_quantumkeeper_multithread();

    virtual SyncPolicy::Type get_thread_type() const override { return SyncPolicy::OS_THREAD; }

    /* cleanly start and stop, with optional 'job' to start */
    virtual void start(std::function<void()> job = nullptr) override;
    virtual void stop() override;

    /* convenience for initiators to evaluate budget */
    virtual sc_core::sc_time time_to_sync() override;

    /* standard TLM2 API */
    void inc(const sc_core::sc_time& t) override;
    void set(const sc_core::sc_time& t) override;
    virtual void sync() override;
    void reset() override;
    sc_core::sc_time get_current_time() const override;
    sc_core::sc_time get_local_time() const override;

    /* non-const need_sync */
    virtual bool need_sync() override;
};
} // namespace gs
#endif // QKMULTITHREAD_H
