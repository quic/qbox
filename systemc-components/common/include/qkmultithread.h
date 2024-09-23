/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QKMULTITHREAD_H
#define QKMULTITHREAD_H

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <mutex>
#include <condition_variable>
#include <systemc>
#include <tlm>
#include <scp/report.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <async_event.h>
#include <qk_extendedif.h>

namespace gs {
// somewhat tuned multiple threaded QK
class tlm_quantumkeeper_multithread : public gs::tlm_quantumkeeper_extended
{
    SCP_LOGGER();
    std::thread::id m_systemc_thread_id;
    std::mutex mutex;
    std::condition_variable cond;
    std::thread m_worker_thread;

protected:
    bool m_systemc_waiting;
    bool m_extern_waiting;
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

    // only NONE, RUNNING and STOPPED will be used by the model, the rest are for debug
    enum jobstates { NONE = 0, RUNNING = 1, STOPPED = 2, SYSC_WAITING = 4, EXT_WAITING = 8, ILLEGAL = 12 } status;

    // MAKE THIS INTO A CCI_PARAM, and provide to_json in the 'normal' way !!!!!

    // this function provided only for debug.
    jobstates get_status() { return (jobstates)(status | (m_systemc_waiting << 2) | (m_extern_waiting << 3)); }
    std::string get_status_json()
    {
        std::string s = "\"name\":\"" + std::string(name()) + "\"";
        if ((status & (RUNNING | STOPPED)) == NONE) s = s + ",\"state\":\"NONE\"";
        if (status & RUNNING) s = s + ",\"state\":\"RUNNING\"";
        if (status & STOPPED) s = s + ",\"state\":\"IDLE\"";
        if (m_systemc_waiting) s = s + ",\"sc_waiting\":true";
        if (m_extern_waiting) s = s + ",\"extern_waiting\":true";
        return "{" + s + "}";
    }
};

static std::vector<gs::tlm_quantumkeeper_multithread*> find_all_tlm_quantumkeeper_multithread()
{
    return find_sc_objects<gs::tlm_quantumkeeper_multithread>();
}

} // namespace gs
#endif // QKMULTITHREAD_H
