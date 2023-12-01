/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PRE_SUSPENDING_SC_SUPPORT_H
#define PRE_SUSPENDING_SC_SUPPORT_H

#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif
#include <systemc>

#if SYSTEMC_VERSION >= 20171012
#define SC_HAS_ASYNC_ATTACH_SUSPENDING
#endif

#if SYSTEMC_VERSION >= 20200101 // NB, this may not be the right number when it's released
#define SC_HAS_SUSPENDING
#endif

#ifndef SC_HAS_SUSPENDING

#include <map>
#include <vector>
#include <condition_variable>

namespace gs {
class global_pause
{
    int m_suspend = 0;
    int m_unsuspendable = 0;
    bool m_has_suspending_channels = false;

    std::map<sc_core::sc_process_b*, bool> m_unsuspendable_map;
    std::map<sc_core::sc_process_b*, bool> m_suspend_all_req_map;
    std::vector<const sc_core::sc_prim_channel*> m_suspending_channels;

    std::condition_variable cond;
    std::mutex mutex;
    int wakeups = 0;

    sc_core::sc_event sleeper_event; // would be convenient to be an async_event
    void sleeper();

public:
    global_pause();
    global_pause(const global_pause&) = delete;

    // (For now) Must be in SystemC thread
    void suspendable();
    void unsuspendable();
    void unsuspend_all();
    // (For now) Must be in SystemC thread
    void suspend_all();
    bool attach_suspending(sc_core::sc_prim_channel* p);
    bool detach_suspending(sc_core::sc_prim_channel* p);

    void async_wakeup();

    static global_pause& get();
};
} // namespace gs
namespace sc_core {
void sc_suspend_all();
void sc_unsuspend_all();
void sc_suspendable();
void sc_unsuspendable();

void sc_internal_async_wakeup();

#ifndef SC_HAS_ASYNC_ATTACH_SUSPENDING
/* Provide a 'global' attach/detach mechanism */
bool async_attach_suspending(sc_core::sc_prim_channel* p);
bool async_detach_suspending(sc_core::sc_prim_channel* p);
#endif
} // namespace sc_core

#endif
#endif // PRE_SUSPENDING_SC_SUPPORT_H
