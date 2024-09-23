/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_MONITOR_H
#define _GREENSOCS_MONITOR_H

/**
 * PythonBinder is not meant to be used as a full systemc/TLM C++ -> python binder,
 * it is used to offload only part of the processing to a python script so that the powerful
 * and rich python language constructs and packages can be used to extend the capabilities
 * of QQVP. So, only the minimum required systemc/TLM classes/functions will have python bindings
 * using pybind11.
 */

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>
#include <scp/helpers.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <ports/initiator-signal-socket.h>
#include <ports/target-signal-socket.h>
#include <tlm_sockets_buswidth.h>
#include <libgssync.h>
#include "crow.h"
#include <thread>
#include <future>

#include <qkmultithread.h>

namespace gs {

template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class monitor : public sc_core::sc_module
{
    SCP_LOGGER();
    using MOD = monitor<BUSWIDTH>;
    using tlm_initiator_socket_t = tlm_utils::simple_initiator_socket_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                        sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_t = tlm_utils::simple_target_socket_tagged_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                         sc_core::SC_ZERO_OR_MORE_BOUND>;

public:
    monitor(const sc_core::sc_module_name& nm);

    ~monitor();

private:
    void init_monitor();

    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans);

    void before_end_of_elaboration() override;

    void end_of_elaboration() override;

    void start_of_simulation() override;

    void end_of_simulation() override;

public:
    cci::cci_param<uint32_t> p_tlm_initiator_ports_num;
    cci::cci_param<uint32_t> p_tlm_target_ports_num;
    cci::cci_param<uint32_t> p_server_port;
    sc_core::sc_vector<tlm_initiator_socket_t> initiator_sockets;
    sc_core::sc_vector<tlm_target_socket_t> target_sockets;

private:
    crow::SimpleApp m_app;
    std::future<void> m_app_future;
    gs::runonsysc m_sc;

    std::vector<tlm_quantumkeeper_multithread*> m_qks;

    const std::string m_html = R"(Use monitor.html)";
};
} // namespace gs

extern "C" void module_register();

#endif
