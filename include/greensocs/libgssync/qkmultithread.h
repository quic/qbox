/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
