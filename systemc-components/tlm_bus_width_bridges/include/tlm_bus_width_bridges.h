/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_TLM_SOCKETS_BUSWIDTH_
#define _GREENSOCS_TLM_SOCKETS_BUSWIDTH_

#include <cci_configuration>
#include <systemc>
#include <scp/report.h>
#include <scp/helpers.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <module_factory_registery.h>
#include <tlm_sockets_buswidth.h>

namespace gs {

template <unsigned int INPUT_BUSWIDTH = DEFAULT_TLM_BUSWIDTH, unsigned int OUTPUT_BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class tlm_bus_width_bridges : public sc_core::sc_module
{
    SCP_LOGGER();

    using MOD = tlm_bus_width_bridges<INPUT_BUSWIDTH, OUTPUT_BUSWIDTH>;
    using tlm_initiator_socket_t = tlm_utils::simple_initiator_socket_b<
        MOD, OUTPUT_BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_t = tlm_utils::simple_target_socket_tagged_b<
        MOD, INPUT_BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;

public:
    tlm_bus_width_bridges(const sc_core::sc_module_name& nm)
        : sc_core::sc_module(nm)
        , initiator_sockets("initiator_socket")
        , target_sockets("target_socket")
        , p_tlm_ports_num("tlm_ports_num", 1, "number of tlm ports")
    {
        SCP_DEBUG(()) << "tlm_bus_width_bridges constructor";
        initiator_sockets.init(p_tlm_ports_num.get_value(),
                               [this](const char* n, int i) { return new tlm_initiator_socket_t(n); });
        target_sockets.init(p_tlm_ports_num.get_value(),
                            [this](const char* n, int i) { return new tlm_target_socket_t(n); });
        for (uint32_t i = 0; i < p_tlm_ports_num.get_value(); i++) {
            target_sockets[i].register_b_transport(this, &MOD::b_transport, i);
            target_sockets[i].register_transport_dbg(this, &MOD::transport_dbg, i);
            target_sockets[i].register_get_direct_mem_ptr(this, &MOD::get_direct_mem_ptr, i);
            initiator_sockets[i].register_invalidate_direct_mem_ptr(this, &MOD::invalidate_direct_mem_ptr);
        }
    }

private:
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        SCP_DEBUG(()) << "calling b_transport: " << scp::scp_txn_tostring(trans);
        initiator_sockets[id]->b_transport(trans, delay);
        SCP_DEBUG(()) << "returning from b_transport: " << scp::scp_txn_tostring(trans);
    }

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        SCP_DEBUG(()) << "calling dbg_transport: " << scp::scp_txn_tostring(trans);
        return initiator_sockets[id]->transport_dbg(trans);
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        SCP_DEBUG(()) << "DMI to " << trans.get_address() << " range " << std::hex << dmi_data.get_start_address()
                      << " - " << std::hex << dmi_data.get_end_address();

        return initiator_sockets[id]->get_direct_mem_ptr(trans, dmi_data);
    }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        SCP_DEBUG(()) << " invalidate_direct_mem_ptr "
                      << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;

        for (long unsigned int i = 0; i < target_sockets.size(); i++) {
            target_sockets[i]->invalidate_direct_mem_ptr(start, end);
        }
    }

public:
    ~tlm_bus_width_bridges() {}

public:
    // These sockets are kept public for binding.
    sc_core::sc_vector<tlm_initiator_socket_t> initiator_sockets;
    sc_core::sc_vector<tlm_target_socket_t> target_sockets;
    cci::cci_param<uint32_t> p_tlm_ports_num;
};
} // namespace gs

extern "C" void module_register();

#endif
