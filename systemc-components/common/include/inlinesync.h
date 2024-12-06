/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INLINESYNC_H
#define INLINESYNC_H

#include <systemc>
#include <tlm>
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/simple_initiator_socket.h"

#include <runonsysc.h>
#include <tlm_sockets_buswidth.h>
#include <module_factory_registery.h>

namespace gs {
class inlinesync : public sc_core::sc_module
{
    runonsysc onSystemC;
    void run_on_sysc(std::function<void()> job) { onSystemC.run_on_sysc(job, true); }

public:
    tlm_utils::simple_target_socket<inlinesync, DEFAULT_TLM_BUSWIDTH> target_socket;
    tlm_utils::simple_initiator_socket<inlinesync, DEFAULT_TLM_BUSWIDTH> initiator_socket;

    SC_HAS_PROCESS(inlinesync);
    inlinesync(const sc_core::sc_module_name& name)
        : sc_module(name), onSystemC(), target_socket("targetSocket"), initiator_socket("initiatorSocket")
    {
        target_socket.register_b_transport(this, &inlinesync::b_transport);
        initiator_socket.register_invalidate_direct_mem_ptr(this, &inlinesync::invalidate_direct_mem_ptr);
    }

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        run_on_sysc([this, &trans, &delay]() {
            sc_core::sc_unsuspendable();
            initiator_socket->b_transport(trans, delay);
            sc_core::sc_suspendable();
        });
    }

    void get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        run_on_sysc([this, &trans, &dmi_data]() { initiator_socket->get_direct_mem_ptr(trans, dmi_data); });
    }
    void transport_dgb(tlm::tlm_generic_payload& trans)
    {
        run_on_sysc([this, &trans]() { initiator_socket->transport_dbg(trans); });
    }
    void invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range)
    {
        // This would be better as run_on_initiator
        target_socket->invalidate_direct_mem_ptr(start_range, end_range);
    }
};

} // namespace gs

#endif // INLINESYNC_H
