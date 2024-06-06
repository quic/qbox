/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __TRANSACTION_FORWARD_IF__
#define __TRANSACTION_FORWARD_IF__

#include <systemc>
#include <tlm>

namespace gs {

enum ForwarderType {
    CONTAINER /*modules container*/,
    PASS /*PassRPC model to work in LOCAL or REMOTE mode*/,
    SC_MODULE /*Generic sc_module*/
};

template <ForwarderType E>
class transaction_forwarder_if
{
public:
    virtual void fw_b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) = 0;

    virtual unsigned int fw_transport_dbg(int id, tlm::tlm_generic_payload& trans) = 0;

    virtual bool fw_get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) = 0;

    virtual void fw_invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) = 0;

    virtual void fw_handle_signal(int id, bool value) = 0;

    virtual ~transaction_forwarder_if() = default;
};
} // namespace gs
#endif