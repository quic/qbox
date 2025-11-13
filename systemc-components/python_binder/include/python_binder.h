/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
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
#include <uutils.h>
#include <pybind11/pybind11.h>
#include <tuple>
#include <unordered_map>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <sstream>

namespace gs {

unsigned char* get_pybind11_buffer_info_ptr(const pybind11::buffer& bytes);

std::string txn_command_str(const tlm::tlm_generic_payload& trans);
class generic_payload_data_buf
{
private:
    tlm::tlm_generic_payload* txn;

public:
    generic_payload_data_buf(tlm::tlm_generic_payload* _txn): txn(_txn) {}
    pybind11::buffer_info get_buffer()
    {
        return pybind11::buffer_info(txn->get_data_ptr(), sizeof(uint8_t),
                                     pybind11::format_descriptor<uint8_t>::format(), 1,
                                     { static_cast<ssize_t>(txn->get_data_length()) }, { sizeof(uint8_t) });
    }
};

class generic_payload_be_buf
{
private:
    tlm::tlm_generic_payload* txn;

public:
    generic_payload_be_buf(tlm::tlm_generic_payload* _txn): txn(_txn) {}
    pybind11::buffer_info get_buffer()
    {
        return pybind11::buffer_info(txn->get_byte_enable_ptr(), sizeof(uint8_t),
                                     pybind11::format_descriptor<uint8_t>::format(), 1,
                                     { static_cast<ssize_t>(txn->get_byte_enable_length()) }, { sizeof(uint8_t) });
    }
};

class tlm_generic_payload_wrapper
{
private:
    tlm::tlm_generic_payload* txn;
    bool txn_created;

public:
    tlm_generic_payload_wrapper(tlm::tlm_generic_payload* _txn): txn(_txn), txn_created(false) {}
    tlm_generic_payload_wrapper(): txn_created(true) { txn = new tlm::tlm_generic_payload(); }
    ~tlm_generic_payload_wrapper()
    {
        if (txn_created) delete txn;
    }

    tlm::tlm_generic_payload* get_payload() { return txn; }

    bool is_read() const { return txn->is_read(); }
    void set_read() { txn->set_read(); }
    bool is_write() const { return txn->is_write(); }
    void set_write() { txn->set_write(); }
    tlm::tlm_command get_command() const { return txn->get_command(); }
    void set_command(const tlm::tlm_command command) { txn->set_command(command); }
    sc_dt::uint64 get_address() const { return txn->get_address(); }
    void set_address(const sc_dt::uint64 address) { txn->set_address(address); }
    void set_data(const pybind11::buffer& bytes)
    {
        unsigned char* data = get_pybind11_buffer_info_ptr(bytes);
        std::memcpy(txn->get_data_ptr(), data, txn->get_data_length());
    }
    generic_payload_data_buf get_data() { return generic_payload_data_buf(txn); }
    void set_data_ptr(const pybind11::buffer& bytes)
    {
        unsigned char* data = get_pybind11_buffer_info_ptr(bytes);
        txn->set_data_ptr(data);
    }
    unsigned int get_data_length() const { return txn->get_data_length(); }
    void set_data_length(const unsigned int length) { txn->set_data_length(length); }
    bool is_response_ok() const { return txn->is_response_ok(); }
    bool is_response_error() const { return txn->is_response_error(); }
    tlm::tlm_response_status get_response_status() const { return txn->get_response_status(); }
    void set_response_status(const tlm::tlm_response_status response_status)
    {
        txn->set_response_status(response_status);
    }
    std::string get_response_string() const { return txn->get_response_string(); };
    unsigned int get_streaming_width() const { return txn->get_streaming_width(); }
    void set_streaming_width(const unsigned int streaming_width) { txn->set_streaming_width(streaming_width); }
    void set_byte_enable_ptr(const pybind11::buffer& bytes)
    {
        unsigned char* byte_enable = get_pybind11_buffer_info_ptr(bytes);
        txn->set_data_ptr(byte_enable);
    }
    void set_byte_enable(const pybind11::buffer& bytes)
    {
        unsigned char* byte_enable = get_pybind11_buffer_info_ptr(bytes);
        std::memcpy(txn->get_byte_enable_ptr(), byte_enable, txn->get_byte_enable_length());
    }
    generic_payload_be_buf get_byte_enable() { return generic_payload_be_buf(txn); }

    unsigned int get_byte_enable_length() const { return txn->get_byte_enable_length(); }
    void set_byte_enable_length(const unsigned int byte_enable_length)
    {
        txn->set_byte_enable_length(byte_enable_length);
    }
    std::string str()
    {
        std::stringstream ss;
        ss << "txn type: " << txn->get_command() << " addr: 0x" << std::hex << txn->get_address()
           << " data len: " << txn->get_data_length() << " response: " << txn->get_response_string();
        return ss.str();
    }
};

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
class python_binder : public sc_core::sc_module, public sc_core::sc_stage_callback_if
{
    SCP_LOGGER();
    using MOD = python_binder<BUSWIDTH>;
    using tlm_initiator_socket_t = tlm_utils::simple_initiator_socket_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                        sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_t = tlm_utils::simple_target_socket_tagged_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                         sc_core::SC_ZERO_OR_MORE_BOUND>;
    using b_transport_info = std::tuple<tlm_generic_payload_wrapper, sc_core::sc_time,
                                        std::shared_ptr<sc_core::sc_event>, std::shared_ptr<sc_core::sc_event>>;
    using b_transport_th_info = std::tuple<int, tlm_generic_payload_wrapper, sc_core::sc_time, bool, std::string,
                                           std::promise<void>>;

public:
    python_binder(const sc_core::sc_module_name& nm);

    ~python_binder();

private:
    void init_binder();

    void setup_biflow_socket(pybind11::object& _modules);

    void do_b_transport(int id, pybind11::object& py_trans, pybind11::object& py_delay);

    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);

    void do_target_b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay,
                               const std::string& tspt_name, std::unordered_map<int, b_transport_info>& cont,
                               bool is_id_used);

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

    void exec_py_b_transport();

    void exec_py_spawned_sc_thread();

    void stage_callback(const sc_core::sc_stage& stage) override;

    void method_thread();

public:
    cci::cci_param<std::string> p_py_mod_name;
    cci::cci_param<std::string> p_py_mod_dir;
    cci::cci_param<std::string> p_py_mod_args;
    cci::cci_param<std::string> p_py_mod_current_mod_id_prefix;
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
    std::unordered_map<int, b_transport_info> m_btspt_cont;
    std::unordered_map<int, b_transport_info> m_bftspt_cont;
    std::unordered_map<int, std::pair<bool, std::shared_ptr<sc_core::sc_event>>> m_target_signals_cont;
    std::unique_ptr<std::thread> m_btspt_method_thread;
    b_transport_th_info m_btspt_info;
    std::condition_variable m_btspt_cv;
    std::mutex m_btspt_mutex;
    bool m_btspt_ready;
    bool m_btspt_thread_stop;
};
} // namespace gs

extern "C" void module_register();

#endif
