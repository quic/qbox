/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_ADDRTR_H
#define _GREENSOCS_BASE_COMPONENTS_ADDRTR_H

#include <cinttypes>
#include <vector>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <cci_configuration>

#include <libgsutils.h>
#include <module_factory_registery.h>
#include <tlm_sockets_buswidth.h>

/**
 * @class Addrtr
 *
 * @brief A Addrtr component that can add addrtr to a virtual platform project to manage the various
 * transactions
 *
 * @details This component models a addrtr. It has a single multi-target socket so any other
 * component with an initiator socket can connect to this component. It behaves as follows:
 *    - Manages exclusive accesses, adding this addrtr as a 'hop' in the exclusive access extension
 * (see GreenSocs/libgsutils).
 *    - Manages connections to multiple initiators and targets with the method `add_initiator` and
 * `add_target`.
 *    - Allows to manage read and write transactions with `b_transport` and `transport_dbg` methods.
 *    - Supports passing through DMI requests with the method `get_direct_mem_ptr`.
 *    - Handles invalidation of multiple DMI pointers with the method `invalidate_direct_mem_ptr`
 * which passes the invalidate back to *all* initiators.
 *    - It checks for each transaction if the address is valid or not and returns an error if the
 * address is invalid with the method `decode_address`.
 */

class addrtr : public sc_core::sc_module
{
private:
    sc_dt::uint64 addr_fw(sc_dt::uint64 addr) { return addr + offset; }
    sc_dt::uint64 addr_bw(sc_dt::uint64 addr) { return addr - offset; }
    void translate_fw(tlm::tlm_generic_payload& trans) { trans.set_address(addr_fw(trans.get_address())); }
    void translate_bw(tlm::tlm_generic_payload& trans) { trans.set_address(addr_bw(trans.get_address())); }

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        translate_fw(trans);
        back_socket->b_transport(trans, delay);
        translate_bw(trans);
    }

    unsigned int transport_dbg(tlm::tlm_generic_payload& trans)
    {
        sc_dt::uint64 addr = trans.get_address();

        translate_fw(trans);
        unsigned int r = back_socket->transport_dbg(trans);
        translate_bw(trans);
        return r;
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        sc_dt::uint64 addr = trans.get_address();

        translate_fw(trans);
        bool r = back_socket->get_direct_mem_ptr(trans, dmi_data);
        translate_bw(trans);
        return r;
    }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        front_socket->invalidate_direct_mem_ptr(addr_bw(start), addr_bw(end));
    }

public:
    tlm_utils::simple_target_socket<addrtr, DEFAULT_TLM_BUSWIDTH> front_socket;
    tlm_utils::simple_initiator_socket<addrtr, DEFAULT_TLM_BUSWIDTH> back_socket;
    cci::cci_param<uint64_t> offset;

    explicit addrtr(const sc_core::sc_module_name& nm)
        : sc_core::sc_module(nm), back_socket("initiator"), front_socket("target"), offset("offset", 0)
    {
        front_socket.register_b_transport(this, &addrtr::b_transport);
        front_socket.register_transport_dbg(this, &addrtr::transport_dbg);
        front_socket.register_get_direct_mem_ptr(this, &addrtr::get_direct_mem_ptr);
        back_socket.register_invalidate_direct_mem_ptr(this, &addrtr::invalidate_direct_mem_ptr);
    }

    addrtr() = delete;

    addrtr(const addrtr&) = delete;

    virtual ~addrtr() {}
};

extern "C" void module_register();

#endif
