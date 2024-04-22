/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_PYTHON_BINDER_DMI_H
#define _GREENSOCS_PYTHON_BINDER_DMI_H

/**
 * PythonBinder is not meant to be used as a full systemc/TLM C++ -> python binder,
 * it is used to offload only part of the processing to a python script so that the powerful
 * and rich python language constructs and packages can be used to extend the capabilities
 * of QQVP. So, only the minimum required systemc/TLM classes/functions will have python bindings
 * using pybind11.
 */

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>
#include <scp/helpers.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <ports/initiator-signal-socket.h>
#include <ports/target-signal-socket.h>
#include <ports/biflow-socket.h>
#include <tlm_sockets_buswidth.h>
#include <libgssync.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>


namespace gs {

class PyInterpreterManager
{
private:
    PyInterpreterManager();

public:
    static void init();

    PyInterpreterManager(PyInterpreterManager const&) = delete;
    void operator=(PyInterpreterManager const&) = delete;

    ~PyInterpreterManager();
};

template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class python_binder : public sc_core::sc_module
{
    SCP_LOGGER();
    using MOD = python_binder<BUSWIDTH>;
    using tlm_initiator_socket_t = tlm_utils::simple_initiator_socket_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                        sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_t = tlm_utils::simple_target_socket_tagged_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                         sc_core::SC_ZERO_OR_MORE_BOUND>;

public:
    python_binder(const sc_core::sc_module_name& nm);

    ~python_binder(){};

private:
    void init_binder();

    void setup_biflow_socket();

    void do_b_transport(int id, pybind11::object& py_trans, pybind11::object& py_delay);

    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);

    void bf_b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay);

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans);

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end);

    void exec_if_py_fn_exist(const char* fn_name);

    void before_end_of_elaboration() override;

    void end_of_elaboration() override;

    void start_of_simulation() override;

    void end_of_simulation() override;

    void target_signal_cb(int id, bool value);

public:
    cci::cci_param<std::string> p_py_mod_name;
    cci::cci_param<std::string> p_py_mod_dir;
    cci::cci_param<std::string> p_py_mod_args;
    cci::cci_param<uint32_t> p_tlm_initiator_ports_num;
    cci::cci_param<uint32_t> p_tlm_target_ports_num;
    cci::cci_param<uint32_t> p_initiator_signals_num;
    cci::cci_param<uint32_t> p_target_signals_num;
    cci::cci_param<uint32_t> p_bf_socket_num;
    sc_core::sc_vector<tlm_initiator_socket_t> initiator_sockets;
    sc_core::sc_vector<tlm_target_socket_t> target_sockets;
    sc_core::sc_vector<InitiatorSignalSocket<bool>> initiator_signal_sockets;
    sc_core::sc_vector<TargetSignalSocket<bool>> target_signal_sockets;
    sc_core::sc_vector<gs::biflow_socket<python_binder<BUSWIDTH>>> bf_socket;

private:
    pybind11::module_ m_main_mod;
    pybind11::module_ m_tlm_do_b_transport_mod;
    pybind11::module_ m_biflow_socket_mod;
    pybind11::module_ m_initiator_signal_socket_mod;
    pybind11::module_ m_cpp_shared_vars_mod;
};
} // namespace gs

extern "C" void module_register();

#endif
