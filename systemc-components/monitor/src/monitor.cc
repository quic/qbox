/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <monitor.h>
#include <module_factory_registery.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <string>
#include <vector>
#include <functional>
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <exception>

namespace gs {

template <unsigned int BUSWIDTH>
monitor<BUSWIDTH>::monitor(const sc_core::sc_module_name& nm)
    : sc_core::sc_module(nm)
    , p_tlm_initiator_ports_num("tlm_initiator_ports_num", 0, "number of tlm initiator ports")
    , p_tlm_target_ports_num("tlm_target_ports_num", 0, "number of tlm target ports")
    , p_server_port("server_port", 18080, "monitor server port number")
    , initiator_sockets("initiator_socket")
    , target_sockets("target_socket")
{
    SCP_DEBUG(()) << "monitor constructor";
    initiator_sockets.init(p_tlm_initiator_ports_num.get_value(),
                           [this](const char* n, int i) { return new tlm_initiator_socket_t(n); });
    target_sockets.init(p_tlm_target_ports_num.get_value(),
                        [this](const char* n, int i) { return new tlm_target_socket_t(n); });

    for (uint32_t i = 0; i < p_tlm_target_ports_num.get_value(); i++) {
        target_sockets[i].register_b_transport(this, &monitor::b_transport, i);
        target_sockets[i].register_transport_dbg(this, &monitor::transport_dbg, i);
    }

    init_monitor();
}

template <unsigned int BUSWIDTH>
monitor<BUSWIDTH>::~monitor()
{
    m_app.stop();
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::init_monitor()
{
    CROW_ROUTE(m_app, "/")
    ([&]() {
        std::string ret = "currently supported methods are:\n/time\n/pause\n/continue\n";
        return ret;
    });
    CROW_ROUTE(m_app, "/time")
    ([&]() {
        std::string ret;
        m_sc.run_on_sysc([&] { ret = "sc timestamp: " + sc_core::sc_time_stamp().to_string() + "\n"; });
        return ret;
    });
    CROW_ROUTE(m_app, "/pause")
    ([&]() {
        std::string ret = "simulation: paused!";
        m_sc.run_on_sysc([&] { sc_core::sc_suspend_all(); });
        return ret;
    });
    CROW_ROUTE(m_app, "/continue")
    ([&]() {
        std::string ret = "simulation: continue";
        m_sc.run_on_sysc([&] { sc_core::sc_unsuspend_all(); });
        return ret;
    });
    m_app_future = m_app.loglevel(crow::LogLevel::Error).port(p_server_port.get_value()).multithreaded().run_async();
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
}

template <unsigned int BUSWIDTH>
unsigned int monitor<BUSWIDTH>::transport_dbg(int id, tlm::tlm_generic_payload& trans)
{
    return 0;
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::before_end_of_elaboration()
{
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::end_of_elaboration()
{
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::start_of_simulation()
{
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::end_of_simulation()
{
}

template class monitor<32>;
template class monitor<64>;
} // namespace gs
typedef gs::monitor<32> monitor;

void module_register() { GSC_MODULE_REGISTER_C(monitor); }
