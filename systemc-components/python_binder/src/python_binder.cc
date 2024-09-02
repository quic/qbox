/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <python_binder.h>
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

using py_char_array = pybind11::array_t<uint8_t, pybind11::array::c_style | pybind11::array::forcecast>;

unsigned char* get_pybind11_buffer_info_ptr(const py_char_array& bytes)
{
    pybind11::buffer_info info(pybind11::buffer(bytes).request());
    unsigned char* ret = reinterpret_cast<unsigned char*>(info.ptr);
    if (!ret) std::cerr << "trying to set transaction data using nullptr!";
    return ret;
}

enum class py_wait_type { SC_TIME, SC_EVENT, GS_ASYNC_EVENT, TIMED_SC_EVENT };

/**
 * FIXME: This is a work around to mitigate a problem happening in python implemented systemc thread functions,
 * now pybind11 will fail to switch from a python function context after a systemc sc_core::wait() call and
 * generates this exception: "loader_life_support: internal error", the current solution assumes that python systemc
 * thread function which includes a sc_core::wait() call is implemented as a generator and yield is used instead
 * of sc_core::wait(), and each time the python systemc thread function yields, a check for the yield value is done to
 * execute the appropriate sc_core::wait() function. So, the actual systemc thread context switch will happen in C++
 * side not in python side.
 */
bool sc_thread_try_wait(const pybind11::object& ret, py_wait_type type)
{
    try {
        switch (type) {
        case py_wait_type::SC_TIME:
            sc_core::wait(ret.cast<const sc_core::sc_time&>());
            break;
        case py_wait_type::SC_EVENT:
            sc_core::wait(ret.cast<const sc_core::sc_event&>());
            break;
        case py_wait_type::GS_ASYNC_EVENT:
            sc_core::wait(ret.cast<const gs::async_event&>());
            break;
        case py_wait_type::TIMED_SC_EVENT: {
            pybind11::tuple t = ret.cast<pybind11::tuple>();
            sc_core::wait(t[0].cast<const sc_core::sc_time&>(), t[1].cast<const sc_core::sc_event&>());
            break;
        }
        default:
            return false;
        }
        return true;
    } catch (const pybind11::reference_cast_error&) {
        return false;
    } catch (const pybind11::cast_error&) {
        return false;
    } catch (pybind11::error_already_set& e) {
        if (!e.matches(PyExc_TypeError)) {
            throw;
        } else {
            return false;
        }
    } catch (...) {
        throw;
    }
}

PYBIND11_EMBEDDED_MODULE(sc_core, m)
{
    m.def("sc_time_stamp", &sc_core::sc_time_stamp);
    m.def("wait", [](const sc_core::sc_time& t) { sc_core::wait(t); });
    m.def("wait", [](const sc_core::sc_event& t) { sc_core::wait(t); });
    m.def("wait", [](const gs::async_event& t) { sc_core::wait(t); });
    m.def("sc_spawn",
          [](std::function<pybind11::object(void)> f, const char* name, const sc_core::sc_spawn_options* opts) {
              sc_core::sc_spawn(
                  [=]() {
                      while (1) {
                          try {
                              pybind11::object ret = f();
                              if (!sc_thread_try_wait(ret, py_wait_type::SC_EVENT) &&
                                  !sc_thread_try_wait(ret, py_wait_type::SC_TIME) &&
                                  !sc_thread_try_wait(ret, py_wait_type::GS_ASYNC_EVENT) &&
                                  !sc_thread_try_wait(ret, py_wait_type::TIMED_SC_EVENT)) {
                                  if (pybind11::str(ret).cast<std::string>() != std::string("None")) {
                                      std::cerr << "Error: unkown sc_core::wait() arguments {" << pybind11::str(ret)
                                                << "}" << std::endl;
                                      abort();
                                  }
                                  return;
                              } else {
                                  continue;
                              }
                          } catch (pybind11::error_already_set& e) {
                              if (!e.matches(PyExc_StopIteration)) {
                                  std::cerr << "Python Exception: " << e.what() << std::endl;
                                  throw;
                              } else {
                                  // No more sc_core::wait(). so the next(thread_generator) will raise StopIteration
                                  // exception.
                                  return;
                              }
                          } catch (const std::exception& ex) {
                              std::cerr << "C++ Exception: " << ex.what() << std::endl;
                              throw;
                          }
                      }
                  },
                  name, opts);
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

    pybind11::enum_<gs::tlm_quantumkeeper_multithread::jobstates>(m, "qk_jobstatus")
        .value("IDLE", gs::tlm_quantumkeeper_multithread::jobstates::NONE)
        .value("IDLE", gs::tlm_quantumkeeper_multithread::jobstates::RUNNING)
        .value("IDLE", gs::tlm_quantumkeeper_multithread::jobstates::STOPPED)
        .value("IDLE", gs::tlm_quantumkeeper_multithread::jobstates::SYSC_WAITING)
        .value("IDLE", gs::tlm_quantumkeeper_multithread::jobstates::EXT_WAITING)
        .value("IDLE", gs::tlm_quantumkeeper_multithread::jobstates::ILLEGAL)
        .export_values();

    m.def("find_all_tlm_quantumkeeper_multithread", &gs::find_all_tlm_quantumkeeper_multithread);

    pybind11::class_<gs::tlm_quantumkeeper_multithread>(m, "qkmultithread")
        .def(pybind11::init<>())
        .def("start", &gs::tlm_quantumkeeper_multithread::start)
        .def("stop", &gs::tlm_quantumkeeper_multithread::stop)
        .def("set", &gs::tlm_quantumkeeper_multithread::set)
        .def("sync", &gs::tlm_quantumkeeper_multithread::sync)
        .def("reset", &gs::tlm_quantumkeeper_multithread::reset)
        .def("get_current_time", &gs::tlm_quantumkeeper_multithread::get_current_time)
        .def("get_local_time", &gs::tlm_quantumkeeper_multithread::get_local_time)
        .def("get_status", &gs::tlm_quantumkeeper_multithread::get_status);
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
                 std::vector<pybind11::ssize_t> shape = { trans.get_data_length() };
                 auto result = py_char_array(shape);
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
                 std::vector<pybind11::ssize_t> shape = { trans.get_byte_enable_length() };
                 auto result = py_char_array(shape);
                 pybind11::buffer_info buf = result.request();
                 std::memcpy(buf.ptr, reinterpret_cast<uint8_t*>(trans.get_byte_enable_ptr()),
                             trans.get_byte_enable_length());
                 return result;
             })
        .def("__repr__", [](tlm::tlm_generic_payload& trans) { return scp::scp_txn_tostring(trans); });
}

PyInterpreterManager::PyInterpreterManager() { pybind11::initialize_interpreter(); }

void PyInterpreterManager::init()
{
    static volatile PyInterpreterManager instance;
    std::cerr << "python interpreter started." << std::endl;
}

PyInterpreterManager::~PyInterpreterManager() { pybind11::finalize_interpreter(); }

template <unsigned int BUSWIDTH>
python_binder<BUSWIDTH>::python_binder(const sc_core::sc_module_name& nm)
    : sc_core::sc_module(nm)
    , p_py_mod_name("py_module_name", "", "name of python script with module implementation")
    , p_py_mod_dir("py_module_dir", "", "path of the directory which contains <py_module_name>.py")
    , p_py_mod_args("py_module_args", "", "a string of command line arguments to be passed to the module")
    , p_py_mod_current_mod_id_prefix("current_mod_id_prefix", "",
                                     "prefix current instance specific module names: [cpp_shared_vars, "
                                     "tlm_do_b_transport, initiator_signal_socket and "
                                     "biflow_socket] with this parameter")
    , p_tlm_initiator_ports_num("tlm_initiator_ports_num", 0, "number of tlm initiator ports")
    , p_tlm_target_ports_num("tlm_target_ports_num", 0, "number of tlm target ports")
    , p_initiator_signals_num("initiator_signals_num", 0, "number of initiator signals")
    , p_target_signals_num("target_signals_num", 0, "number of target signals")
    , p_bf_socket_num("biflow_socket_num", 0, "number of biflow sockets, maximum 1 socket is supported")
    , initiator_sockets("initiator_socket")
    , target_sockets("target_socket")
    , initiator_signal_sockets("initiator_signal_socket")
    , target_signal_sockets("target_signal_socket")
    , bf_socket("biflow_socket")
{
    SCP_DEBUG(()) << "python_binder constructor";
    assert(p_bf_socket_num.get_value() == 0 || p_bf_socket_num.get_value() == 1);
    initiator_sockets.init(p_tlm_initiator_ports_num.get_value(),
                           [this](const char* n, int i) { return new tlm_initiator_socket_t(n); });
    target_sockets.init(p_tlm_target_ports_num.get_value(),
                        [this](const char* n, int i) { return new tlm_target_socket_t(n); });
    bf_socket.init(p_bf_socket_num.get_value(),
                   [this](const char* n, int i) { return new gs::biflow_socket<python_binder<BUSWIDTH>>(n); });
    initiator_signal_sockets.init(p_initiator_signals_num.get_value(),
                                  [this](const char* n, int i) { return new InitiatorSignalSocket<bool>(n); });
    target_signal_sockets.init(p_target_signals_num.get_value(),
                               [this](const char* n, int i) { return new TargetSignalSocket<bool>(n); });
    for (uint32_t i = 0; i < p_tlm_target_ports_num.get_value(); i++) {
        target_sockets[i].register_b_transport(this, &python_binder::b_transport, i);
        target_sockets[i].register_transport_dbg(this, &python_binder::transport_dbg, i);
        target_sockets[i].register_get_direct_mem_ptr(this, &python_binder::get_direct_mem_ptr, i);
    }
    if (p_bf_socket_num.get_value() == 1) bf_socket[0].register_b_transport(this, &python_binder::bf_b_transport);
    for (uint32_t i = 0; i < p_tlm_initiator_ports_num.get_value(); i++) {
        initiator_sockets[i].register_invalidate_direct_mem_ptr(this, &python_binder::invalidate_direct_mem_ptr);
    }
    for (uint32_t i = 0; i < p_target_signals_num.get_value(); i++) {
        target_signal_sockets[i].register_value_changed_cb([this, i](bool value) { target_signal_cb(i, value); });
    }

    init_binder();
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::init_binder()
{
    if (p_py_mod_dir.get_value().empty() || p_py_mod_name.get_value().empty()) {
        SCP_FATAL(()) << "py_module_dir and py_module_name parameters shouldn't be empty!";
    }
    try {
        PyInterpreterManager::init();

        pybind11::module_ sys_mod = pybind11::module_::import("sys");
        pybind11::object modules = sys_mod.attr("modules");
        pybind11::object path = sys_mod.attr("path");
        pybind11::object append = path.attr("append");
        append(p_py_mod_dir.get_value().c_str());
        pybind11::print("current python path: ", path);

        // create dyamic modules
        std::string m_cpp_shared_vars_mod_name = p_py_mod_current_mod_id_prefix.get_value() +
                                                 std::string("cpp_shared_vars");
        m_cpp_shared_vars_mod = pybind11::module_::import("types").attr("ModuleType")(
            m_cpp_shared_vars_mod_name.c_str());
        m_cpp_shared_vars_mod.attr("module_args") = p_py_mod_args.get_value();
        modules.attr("setdefault")(m_cpp_shared_vars_mod_name.c_str(), m_cpp_shared_vars_mod);

        std::string m_tlm_do_b_transport_mod_name = p_py_mod_current_mod_id_prefix.get_value() +
                                                    std::string("tlm_do_b_transport");
        m_tlm_do_b_transport_mod = pybind11::module_::import("types").attr("ModuleType")(
            m_tlm_do_b_transport_mod_name.c_str());
        m_tlm_do_b_transport_mod.attr("do_b_transport") = pybind11::cpp_function(
            [this](int id, pybind11::object& py_trans, pybind11::object& delay) {
                do_b_transport(id, py_trans, delay);
            });
        modules.attr("setdefault")(m_tlm_do_b_transport_mod_name.c_str(), m_tlm_do_b_transport_mod);

        setup_biflow_socket(modules);

        std::string m_initiator_signal_socket_mod_name = p_py_mod_current_mod_id_prefix.get_value() +
                                                         std::string("initiator_signal_socket");
        m_initiator_signal_socket_mod = pybind11::module_::import("types").attr("ModuleType")(
            m_initiator_signal_socket_mod_name.c_str());
        m_initiator_signal_socket_mod.attr("write") = pybind11::cpp_function(
            [this](int id, bool value) { initiator_signal_sockets[id]->write(value); });
        modules.attr("setdefault")(m_initiator_signal_socket_mod_name.c_str(), m_initiator_signal_socket_mod);

        // import the python model
        std::string module_name_param = p_py_mod_name.get_value();
        std::string py_suffix = ".py";
        std::string module_name_wo_suffix = (module_name_param.size() > py_suffix.size() &&
                                             module_name_param.compare(module_name_param.size() - py_suffix.size(),
                                                                       py_suffix.size(), py_suffix) == 0)
                                                ? module_name_param.substr(0,
                                                                           module_name_param.size() - py_suffix.size())
                                                : module_name_param;
        const char* module_name = module_name_wo_suffix.c_str();
        m_main_mod = pybind11::module_::import(module_name);
    } catch (const std::exception& e) {
        SCP_FATAL(()) << e.what() << std::endl;
    }
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::setup_biflow_socket(pybind11::object& _modules)
{
    std::string m_biflow_socket_mod_name = p_py_mod_current_mod_id_prefix.get_value() + std::string("biflow_socket");
    m_biflow_socket_mod = pybind11::module_::import("types").attr("ModuleType")(m_biflow_socket_mod_name.c_str());
    m_biflow_socket_mod.attr("can_receive_more") = pybind11::cpp_function(
        [this](int i) { bf_socket[0].can_receive_more(i); });
    m_biflow_socket_mod.attr("can_receive_set") = pybind11::cpp_function(
        [this](int i) { bf_socket[0].can_receive_set(i); });
    m_biflow_socket_mod.attr("can_receive_any") = pybind11::cpp_function([this]() { bf_socket[0].can_receive_any(); });
    m_biflow_socket_mod.attr("enqueue") = pybind11::cpp_function([this](uint8_t data) { bf_socket[0].enqueue(data); });
    m_biflow_socket_mod.attr("set_default_txn") = pybind11::cpp_function([this](pybind11::object& py_trans) {
        tlm::tlm_generic_payload* trans = py_trans.cast<tlm::tlm_generic_payload*>();
        bf_socket[0].set_default_txn(*trans);
    });
    m_biflow_socket_mod.attr("force_send") = pybind11::cpp_function([this](pybind11::object& py_trans) {
        tlm::tlm_generic_payload* trans = py_trans.cast<tlm::tlm_generic_payload*>();
        bf_socket[0].force_send(*trans);
    });
    m_biflow_socket_mod.attr("reset") = pybind11::cpp_function([this]() { bf_socket[0].reset(); });
    _modules.attr("setdefault")(m_biflow_socket_mod_name.c_str(), m_biflow_socket_mod);
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::do_b_transport(int id, pybind11::object& py_trans, pybind11::object& py_delay)
{
    tlm::tlm_generic_payload* trans = py_trans.cast<tlm::tlm_generic_payload*>();
    sc_core::sc_time* delay = py_delay.cast<sc_core::sc_time*>();
    SCP_DEBUG(()) << "do_b_transport using initiator_socket_" << id << " trans: " << scp::scp_txn_tostring(*trans);
    initiator_sockets[id]->b_transport(*trans, *delay);
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
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
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::bf_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
    SCP_DEBUG(()) << "before bf_b_transport"
                  << " trans: " << scp::scp_txn_tostring(trans);
    tlm::tlm_generic_payload* ptrans = &trans;
    sc_core::sc_time* pdelay = &delay;
    try {
        m_main_mod.attr("bf_b_transport")(pybind11::cast(ptrans), pybind11::cast(pdelay));
    } catch (const std::exception& e) {
        SCP_FATAL(()) << e.what();
    }
    SCP_DEBUG(()) << "after bf_b_transport "
                  << " trans: " << scp::scp_txn_tostring(trans);
}

template <unsigned int BUSWIDTH>
unsigned int python_binder<BUSWIDTH>::transport_dbg(int id, tlm::tlm_generic_payload& trans)
{
    return 0;
}

template <unsigned int BUSWIDTH>
bool python_binder<BUSWIDTH>::get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
{
    return false;
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
{
    SCP_DEBUG(()) << " invalidate_direct_mem_ptr "
                  << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;
    for (uint32_t i = 0; i < target_sockets.size(); i++) {
        target_sockets[i]->invalidate_direct_mem_ptr(start, end);
    }
}

/**
 * let's be less strict about the existence check of these callbacks in the python module.
 */

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::exec_if_py_fn_exist(const char* fn_name)
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

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::before_end_of_elaboration()
{
    exec_if_py_fn_exist("before_end_of_elaboration");
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::end_of_elaboration()
{
    exec_if_py_fn_exist("end_of_elaboration");
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::start_of_simulation()
{
    exec_if_py_fn_exist("start_of_simulation");
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::end_of_simulation()
{
    exec_if_py_fn_exist("end_of_simulation");
}

template <unsigned int BUSWIDTH>
void python_binder<BUSWIDTH>::target_signal_cb(int id, bool value)
{
    try {
        m_main_mod.attr("target_signal_cb")(id, value);
    } catch (const std::exception& e) {
        SCP_FATAL(()) << e.what();
    }
}

template class python_binder<32>;
template class python_binder<64>;
} // namespace gs
typedef gs::python_binder<32> python_binder;

void module_register() { GSC_MODULE_REGISTER_C(python_binder); }
