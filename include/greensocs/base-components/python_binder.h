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
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <greensocs/gsutils/ports/target-signal-socket.h>
#include <greensocs/gsutils/module_factory_registery.h>
#include <greensocs/libgssync.h>
#include <cstdint>
#include <exception>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <string>
#include <vector>
#include <functional>
#include <exception>
#include <stdexcept>

#define __BUSWIDTH__ 32

namespace gs {

using py_char_array = pybind11::array_t<uint8_t, pybind11::array::c_style | pybind11::array::forcecast>;

unsigned char* get_pybind11_buffer_info_ptr(const py_char_array& bytes)
{
    pybind11::buffer_info info(pybind11::buffer(bytes).request());
    unsigned char* ret = reinterpret_cast<unsigned char*>(info.ptr);
    if (!ret) std::cerr << "trying to set transaction data using nullptr!";
    return ret;
}

PYBIND11_EMBEDDED_MODULE(sc_core, m)
{
    m.def("wait", [](const sc_core::sc_time& t) { sc_core::wait(t); });
    m.def("wait", [](const sc_core::sc_event& t) { sc_core::wait(t); });
    m.def("wait", [](const gs::async_event& t) { sc_core::wait(t); });
    m.def("sc_spawn", [](std::function<void()> f, const char* name, const sc_core::sc_spawn_options* opts) {
        sc_core::sc_spawn(f, name, opts);
    });

    pybind11::enum_<sc_core::sc_time_unit>(m, "sc_time_unit")
        .value("SC_FS", sc_core::sc_time_unit::SC_FS)
        .value("SC_PS", sc_core::sc_time_unit::SC_PS)
        .value("SC_NS", sc_core::sc_time_unit::SC_NS)
        .value("SC_US", sc_core::sc_time_unit::SC_US)
        .value("SC_MS", sc_core::sc_time_unit::SC_MS)
        .value("SC_SEC", sc_core::sc_time_unit::SC_SEC)
        .export_values();

    pybind11::class_<sc_core::sc_time>(m, "sc_time")
        .def(pybind11::init<>())
        .def(pybind11::init<double, sc_core::sc_time_unit>())
        .def(pybind11::init<double, const char*>())
        .def(pybind11::init<double, bool>())
        .def(pybind11::init<sc_core::sc_time::value_type, bool>())
        .def("__copy__", [](const sc_core::sc_time& self) { return sc_core::sc_time(self); })
        .def("from_seconds", &sc_core::sc_time::from_seconds)
        .def("from_value", &sc_core::sc_time::from_value)
        .def("from_string", &sc_core::sc_time::from_string)
        .def("to_default_time_units", &sc_core::sc_time::to_default_time_units)
        .def("to_double", &sc_core::sc_time::to_double)
        .def("to_seconds", &sc_core::sc_time::to_seconds)
        .def("to_string", &sc_core::sc_time::to_string)
        .def("__repr__", &sc_core::sc_time::to_string)
        .def("value", &sc_core::sc_time::value)
        .def(pybind11::self + pybind11::self)
        .def(pybind11::self - pybind11::self)
        .def(pybind11::self / pybind11::self)
        .def(pybind11::self / double())
        .def(pybind11::self * double())
        .def(double() * pybind11::self)
        .def(pybind11::self % pybind11::self)
        .def(pybind11::self += pybind11::self)
        .def(pybind11::self -= pybind11::self)
        .def(pybind11::self %= pybind11::self)
        .def(pybind11::self *= double())
        .def(pybind11::self /= double())
        .def(pybind11::self == pybind11::self)
        .def(pybind11::self != pybind11::self)
        .def(pybind11::self <= pybind11::self)
        .def(pybind11::self >= pybind11::self)
        .def(pybind11::self > pybind11::self)
        .def(pybind11::self < pybind11::self);

    pybind11::class_<sc_core::sc_event>(m, "sc_event")
        .def(pybind11::init<>())
        .def(pybind11::init<const char*>())
        .def("notify", static_cast<void (sc_core::sc_event::*)()>(&sc_core::sc_event::notify))
        .def("notify", static_cast<void (sc_core::sc_event::*)(const sc_core::sc_time&)>(&sc_core::sc_event::notify))
        .def("notify",
             static_cast<void (sc_core::sc_event::*)(double, sc_core::sc_time_unit)>(&sc_core::sc_event::notify))
        .def("notify_delayed", static_cast<void (sc_core::sc_event::*)()>(&sc_core::sc_event::notify_delayed))
        .def("notify_delayed",
             static_cast<void (sc_core::sc_event::*)(const sc_core::sc_time&)>(&sc_core::sc_event::notify_delayed))
        .def("notify_delayed", static_cast<void (sc_core::sc_event::*)(double, sc_core::sc_time_unit)>(
                                   &sc_core::sc_event::notify_delayed));

    pybind11::class_<sc_core::sc_spawn_options>(m, "sc_spawn_options")
        .def(pybind11::init<>())
        .def("dont_initialize", &sc_core::sc_spawn_options::dont_initialize)
        .def("is_method", &sc_core::sc_spawn_options::is_method)
        .def("set_stack_size", &sc_core::sc_spawn_options::set_stack_size)
        .def("set_sensitivity",
             [](sc_core::sc_spawn_options& self, const gs::async_event* ev) { self.set_sensitivity(ev); })
        .def("set_sensitivity", static_cast<void (sc_core::sc_spawn_options::*)(const sc_core::sc_event*)>(
                                    &sc_core::sc_spawn_options::set_sensitivity))
        .def("spawn_method", &sc_core::sc_spawn_options::spawn_method);
}

PYBIND11_EMBEDDED_MODULE(gs, m)
{
    pybind11::class_<gs::async_event>(m, "async_event")
        .def(pybind11::init<bool>())
        .def("async_notify", &gs::async_event::async_notify)
        .def("notify", &gs::async_event::notify)
        .def("async_attach_suspending", &gs::async_event::async_attach_suspending)
        .def("async_detach_suspending", &gs::async_event::async_detach_suspending)
        .def("enable_attach_suspending", &gs::async_event::enable_attach_suspending);
}

PYBIND11_EMBEDDED_MODULE(tlm_generic_payload, m)
{
    pybind11::enum_<tlm::tlm_command>(m, "tlm_command")
        .value("TLM_READ_COMMAND", tlm::tlm_command::TLM_READ_COMMAND)
        .value("TLM_WRITE_COMMAND", tlm::tlm_command::TLM_WRITE_COMMAND)
        .value("TLM_IGNORE_COMMAND", tlm::tlm_command::TLM_IGNORE_COMMAND)
        .export_values();

    pybind11::enum_<tlm::tlm_response_status>(m, "tlm_response_status")
        .value("TLM_OK_RESPONSE", tlm::tlm_response_status::TLM_OK_RESPONSE)
        .value("TLM_INCOMPLETE_RESPONSE", tlm::tlm_response_status::TLM_INCOMPLETE_RESPONSE)
        .value("TLM_GENERIC_ERROR_RESPONSE", tlm::tlm_response_status::TLM_GENERIC_ERROR_RESPONSE)
        .value("TLM_ADDRESS_ERROR_RESPONSE", tlm::tlm_response_status::TLM_ADDRESS_ERROR_RESPONSE)
        .value("TLM_COMMAND_ERROR_RESPONSE", tlm::tlm_response_status::TLM_COMMAND_ERROR_RESPONSE)
        .value("TLM_BURST_ERROR_RESPONSE", tlm::tlm_response_status::TLM_BURST_ERROR_RESPONSE)
        .value("TLM_BYTE_ENABLE_ERROR_RESPONSE", tlm::tlm_response_status::TLM_BYTE_ENABLE_ERROR_RESPONSE)
        .export_values();

    pybind11::class_<tlm::tlm_generic_payload>(m, "tlm_generic_payload")
        .def(pybind11::init<>())
        .def("get_address", &tlm::tlm_generic_payload::get_address)
        .def("set_address", &tlm::tlm_generic_payload::set_address)
        .def("is_read", &tlm::tlm_generic_payload::is_read)
        .def("is_write", &tlm::tlm_generic_payload::is_write)
        .def("set_read", &tlm::tlm_generic_payload::set_read)
        .def("set_write", &tlm::tlm_generic_payload::set_write)
        .def("get_command", &tlm::tlm_generic_payload::get_command)
        .def("set_command", &tlm::tlm_generic_payload::set_command)
        .def("is_response_ok", &tlm::tlm_generic_payload::is_response_ok)
        .def("is_response_error", &tlm::tlm_generic_payload::is_response_error)
        .def("get_response_status", &tlm::tlm_generic_payload::get_response_status)
        .def("set_response_status", &tlm::tlm_generic_payload::set_response_status)
        .def("get_response_string", &tlm::tlm_generic_payload::get_response_string)
        .def("get_streaming_width", &tlm::tlm_generic_payload::get_streaming_width)
        .def("set_streaming_width", &tlm::tlm_generic_payload::set_streaming_width)
        .def("set_data_length", &tlm::tlm_generic_payload::set_data_length)
        .def("get_data_length", &tlm::tlm_generic_payload::get_data_length)
        // when creating new transaction from python.
        .def("set_data_ptr",
             [](tlm::tlm_generic_payload& trans, py_char_array& bytes) {
                 unsigned char* data = get_pybind11_buffer_info_ptr(bytes);
                 trans.set_data_ptr(data);
             })
        .def("set_data",
             [](tlm::tlm_generic_payload& trans, py_char_array& bytes) {
                 unsigned char* data = get_pybind11_buffer_info_ptr(bytes);
                 std::memcpy(trans.get_data_ptr(), data, trans.get_data_length());
             })
        .def("get_data",
             [](tlm::tlm_generic_payload& trans) -> py_char_array {
                 // https://pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html
                 auto result = py_char_array(trans.get_data_length());
                 pybind11::buffer_info buf = result.request();
                 std::memcpy(buf.ptr, reinterpret_cast<uint8_t*>(trans.get_data_ptr()), trans.get_data_length());
                 return result;
             })
        .def("set_byte_enable_length", &tlm::tlm_generic_payload::set_byte_enable_length)
        .def("get_byte_enable_length", &tlm::tlm_generic_payload::get_byte_enable_length)
        .def("set_byte_enable_ptr",
             [](tlm::tlm_generic_payload& trans, py_char_array& bytes) {
                 unsigned char* byte_enable = get_pybind11_buffer_info_ptr(bytes);
                 trans.set_data_ptr(byte_enable);
             })
        .def("set_byte_enable",
             [](tlm::tlm_generic_payload& trans, py_char_array& bytes) {
                 unsigned char* byte_enable = get_pybind11_buffer_info_ptr(bytes);
                 std::memcpy(trans.get_byte_enable_ptr(), byte_enable, trans.get_byte_enable_length());
             })
        .def("get_byte_enable",
             [](tlm::tlm_generic_payload& trans) -> py_char_array {
                 auto result = py_char_array(trans.get_byte_enable_length());
                 pybind11::buffer_info buf = result.request();
                 std::memcpy(buf.ptr, reinterpret_cast<uint8_t*>(trans.get_byte_enable_ptr()),
                             trans.get_byte_enable_length());
                 return result;
             })
        .def("__repr__", [](tlm::tlm_generic_payload& trans) { return scp::scp_txn_tostring(trans); });
}

PYBIND11_EMBEDDED_MODULE(cpp_shared_vars, m) { m.attr("module_args") = std::string(""); }

PYBIND11_EMBEDDED_MODULE(tlm_do_b_transport, m) {}

PYBIND11_EMBEDDED_MODULE(initiator_signal_socket, m) {}

class PyInterpreterManager
{
private:
    PyInterpreterManager() { pybind11::initialize_interpreter(); }

public:
    static void init()
    {
        static volatile PyInterpreterManager instance;
        std::cerr << "python interpreter started." << std::endl;
    }

    PyInterpreterManager(PyInterpreterManager const&) = delete;
    void operator=(PyInterpreterManager const&) = delete;

    ~PyInterpreterManager() { pybind11::finalize_interpreter(); }
};

template <unsigned int BUSWIDTH = __BUSWIDTH__>
class PythonBinder : public sc_core::sc_module
{
    SCP_LOGGER();
    using MOD = PythonBinder<BUSWIDTH>;
    using tlm_initiator_socket_t = tlm_utils::simple_initiator_socket_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                        sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_t = tlm_utils::simple_target_socket_tagged_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                         sc_core::SC_ZERO_OR_MORE_BOUND>;

public:
    PythonBinder(const sc_core::sc_module_name& nm)
        : sc_core::sc_module(nm)
        , p_py_mod_name("py_module_name", "", "name of python script with module implementation")
        , p_py_mod_dir("py_module_dir", "", "path of the directory which contains <py_module_name>.py")
        , p_py_mod_args("py_module_args", "", "a string of command line arguments to be passed to the module")
        , p_tlm_initiator_ports_num("tlm_initiator_ports_num", 0, "number of tlm initiator ports")
        , p_tlm_target_ports_num("tlm_target_ports_num", 0, "number of tlm target ports")
        , p_initiator_signals_num("initiator_signals_num", 0, "number of initiator signals")
        , p_target_signals_num("target_signals_num", 0, "number of target signals")
        , initiator_sockets("initiator_socket")
        , target_sockets("target_socket")
        , initiator_signal_sockets("initiator_signal_socket")
        , target_signal_sockets("target_signal_socket")
    {
        SCP_DEBUG(()) << "PythonBinder constructor";
        initiator_sockets.init(p_tlm_initiator_ports_num.get_value(),
                               [this](const char* n, int i) { return new tlm_initiator_socket_t(n); });
        target_sockets.init(p_tlm_target_ports_num.get_value(),
                            [this](const char* n, int i) { return new tlm_target_socket_t(n); });
        initiator_signal_sockets.init(p_initiator_signals_num.get_value(),
                                      [this](const char* n, int i) { return new InitiatorSignalSocket<bool>(n); });
        target_signal_sockets.init(p_target_signals_num.get_value(),
                                   [this](const char* n, int i) { return new TargetSignalSocket<bool>(n); });

        for (int i = 0; i < p_tlm_target_ports_num.get_value(); i++) {
            target_sockets[i].register_b_transport(this, &PythonBinder::b_transport, i);
            target_sockets[i].register_transport_dbg(this, &PythonBinder::transport_dbg, i);
            target_sockets[i].register_get_direct_mem_ptr(this, &PythonBinder::get_direct_mem_ptr, i);
        }
        for (int i = 0; i < p_tlm_initiator_ports_num.get_value(); i++) {
            initiator_sockets[i].register_invalidate_direct_mem_ptr(this, &PythonBinder::invalidate_direct_mem_ptr);
        }
        for (int i = 0; i < p_target_signals_num.get_value(); i++) {
            target_signal_sockets[i].register_value_changed_cb([this, i](bool value) { target_signal_cb(i, value); });
        }

        init_binder();
    }

    ~PythonBinder(){};

private:
    void init_binder()
    {
        if (p_py_mod_dir.get_value().empty() || p_py_mod_name.get_value().empty()) {
            SCP_FATAL(()) << "py_module_dir and py_module_name parameters shouldn't be empty!";
        }
        try {
            PyInterpreterManager::init();

            pybind11::object path = pybind11::module_::import("sys").attr("path");
            pybind11::object append = path.attr("append");
            append(p_py_mod_dir.get_value().c_str());
            pybind11::print("current python path: ", path);

            m_cpp_shared_vars_mod = pybind11::module_::import("cpp_shared_vars");
            m_cpp_shared_vars_mod.attr("module_args") = p_py_mod_args.get_value();

            m_tlm_do_b_transport_mod = pybind11::module_::import("tlm_do_b_transport");
            m_tlm_do_b_transport_mod.attr("do_b_transport") = pybind11::cpp_function(
                [this](int id, pybind11::object& py_trans, pybind11::object& delay) {
                    do_b_transport(id, py_trans, delay);
                });

            m_initiator_signal_socket_mod = pybind11::module_::import("initiator_signal_socket");
            m_initiator_signal_socket_mod.attr("write") = pybind11::cpp_function(
                [this](int id, bool value) { initiator_signal_sockets[id]->write(value); });

            std::string module_name_param = p_py_mod_name.get_value();
            std::string py_suffix = ".py";
            std::string module_name_wo_suffix = (module_name_param.size() > py_suffix.size() &&
                                                 module_name_param.compare(module_name_param.size() - py_suffix.size(),
                                                                           py_suffix.size(), py_suffix) == 0)
                                                    ? module_name_param.substr(
                                                          0, module_name_param.size() - py_suffix.size())
                                                    : module_name_param;
            const char* module_name = module_name_wo_suffix.c_str();
            m_main_mod = pybind11::module_::import(module_name);
        } catch (const std::exception& e) {
            SCP_FATAL(()) << e.what() << std::endl;
        }
    }

    void do_b_transport(int id, pybind11::object& py_trans, pybind11::object& py_delay)
    {
        tlm::tlm_generic_payload* trans = py_trans.cast<tlm::tlm_generic_payload*>();
        sc_core::sc_time* delay = py_delay.cast<sc_core::sc_time*>();
        SCP_DEBUG(()) << "do_b_transport using initiator_socket_" << id << " trans: " << scp::scp_txn_tostring(*trans);
        initiator_sockets[id]->b_transport(*trans, *delay);
    }

    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        SCP_DEBUG(()) << "before b_transport on target_socket_" << id << " trans: " << scp::scp_txn_tostring(trans);
        tlm::tlm_generic_payload* ptrans = &trans;
        sc_core::sc_time* pdelay = &delay;
        try {
            m_main_mod.attr("b_transport")(id, pybind11::cast(ptrans), pybind11::cast(pdelay));
        } catch (const std::exception& e) {
            SCP_FATAL(()) << e.what();
        }
        SCP_DEBUG(()) << "after b_transport on target_socket_" << id << " trans: " << scp::scp_txn_tostring(trans);
        ;
    }

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans) { return 0; }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) { return false; }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        SCP_DEBUG(()) << " invalidate_direct_mem_ptr "
                      << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;
        for (int i = 0; i < target_sockets.size(); i++) {
            target_sockets[i]->invalidate_direct_mem_ptr(start, end);
        }
    }

    /**
     * let's be less strict about the existence check of these callbacks in the python module.
     */

    void exec_if_py_fn_exist(const char* fn_name)
    {
        try {
            m_main_mod.attr(fn_name)();
        } catch (pybind11::error_already_set& ex) {
            if (!ex.matches(PyExc_AttributeError))
                throw;
            else
                SCP_DEBUG(()) << fn_name << "() is not implemented in " << p_py_mod_name.get_value();
        } catch (const std::exception& e) {
            SCP_FATAL(()) << e.what();
        }
    }

    void before_end_of_elaboration() override { exec_if_py_fn_exist("before_end_of_elaboration"); }

    virtual void end_of_elaboration() override { exec_if_py_fn_exist("end_of_elaboration"); }

    virtual void start_of_simulation() override { exec_if_py_fn_exist("start_of_simulation"); }

    virtual void end_of_simulation() override { exec_if_py_fn_exist("end_of_simulation"); }

    void target_signal_cb(int id, bool value)
    {
        try {
            m_main_mod.attr("target_signal_cb")(id, value);
        } catch (const std::exception& e) {
            SCP_FATAL(()) << e.what();
        }
    }

public:
    cci::cci_param<std::string> p_py_mod_name;
    cci::cci_param<std::string> p_py_mod_dir;
    cci::cci_param<std::string> p_py_mod_args;
    cci::cci_param<uint32_t> p_tlm_initiator_ports_num;
    cci::cci_param<uint32_t> p_tlm_target_ports_num;
    cci::cci_param<uint32_t> p_initiator_signals_num;
    cci::cci_param<uint32_t> p_target_signals_num;
    sc_core::sc_vector<tlm_initiator_socket_t> initiator_sockets;
    sc_core::sc_vector<tlm_target_socket_t> target_sockets;
    sc_core::sc_vector<InitiatorSignalSocket<bool>> initiator_signal_sockets;
    sc_core::sc_vector<TargetSignalSocket<bool>> target_signal_sockets;

private:
    pybind11::module_ m_main_mod;
    pybind11::module_ m_tlm_do_b_transport_mod;
    pybind11::module_ m_initiator_signal_socket_mod;
    pybind11::module_ m_cpp_shared_vars_mod;
};
} // namespace gs
typedef gs::PythonBinder<> PythonBinder;
GSC_MODULE_REGISTER(PythonBinder);
#endif
