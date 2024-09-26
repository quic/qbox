/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define SC_ALLOW_DEPRECATED_IEEE_API

/**
 * Example configuration:
 *
 * platform["qmp_0"] = {
    moduletype = "qmp";
    args = {"&platform.qemu_inst"};
    qmp_str = "unix:qmp.sock,server,nowait";
};

platform["monitor_0"] = {
    moduletype = "monitor";
    server_port = 18080;
    use_qmp = true;
    qmp_ud_socket_path  = "qmp.sock";
    html_doc_template_dir_path = "/path/to/html/templates";
    html_doc_name = "monitor.html";
};
 */

#include <monitor.h>
#include <module_factory_registery.h>
#include <vector>
#include <functional>
#include <exception>
#include <cciutils.h>
#include <algorithm>

namespace gs {

template <unsigned int BUSWIDTH>
monitor<BUSWIDTH>::monitor(const sc_core::sc_module_name& nm)
    : sc_core::sc_module(nm)
    , p_tlm_initiator_ports_num("tlm_initiator_ports_num", 0, "number of tlm initiator ports")
    , p_tlm_target_ports_num("tlm_target_ports_num", 0, "number of tlm target ports")
    , p_server_port("server_port", 18080, "monitor server port number")
    , p_use_qmp("use_qmp", false, "is qmp model used?")
    , p_qmp_ud_socket_path("qmp_ud_socket_path", "qmp.sock", "QMP unix domain socket path")
    , p_html_doc_template_dir_path("html_doc_template_dir_path", "",
                                   "path to a template directory where HTML document to call the REST API exist")
    , p_html_doc_name("html_doc_name", "", "name of a HTML document to call the REST API")
    , initiator_sockets("initiator_socket")
    , target_sockets("target_socket")
    , m_u_endpoint(p_qmp_ud_socket_path.get_value())
    , m_u_socket(m_ios)
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
    m_u_socket.close();
}

std::vector<crow::json::wvalue> json_cci_params(sc_core::sc_object* obj)
{
    std::string name = obj->name();
    cci_broker_handle m_broker = (sc_core::sc_get_current_object())
                                     ? cci_get_broker()
                                     : cci_get_global_broker(cci_originator("gs__sc_cci_children"));
    std::set<std::string> cldr;
    for (sc_core::sc_object* child : obj->get_child_objects()) {
        if (child->kind() == std::string("sc_module")) {
            cldr.insert(child->name());
        }
    }

    int l = name.length() + 1;
    auto uncon = m_broker.get_unconsumed_preset_values([&](const std::pair<std::string, cci_value>& iv) {
        if (iv.first.find(name + ".") == 0) {
            std::string namep = iv.first.substr(0, iv.first.find(".", l));
            return (cldr.find(namep) == cldr.end());
        }
        return false;
    });
    cci_param_predicate pred([&](const cci_param_untyped_handle& p) {
        std::string p_name = p.name();
        if (p_name.find(name + ".") == 0) {
            std::string namep = p_name.substr(0, p_name.find(".", l));
            return (cldr.find(namep) == cldr.end());
        }
        return false;
    });
    auto cons = m_broker.get_param_handles(pred);

    std::vector<crow::json::wvalue> children;
    for (auto p : uncon) {
        children.push_back({ { "name", p.first.substr(l) },
                             { "value", crow::json::load(p.second.to_json()) },
                             { "basename", p.first } });
    }
    for (auto p : cons) {
        children.push_back({ { "basename", std::string(p.name()).substr(l) },
                             { "value", crow::json::load(p.get_cci_value().to_json()) },
                             { "name", p.name() },
                             { "description", p.get_description() } });
    }

    return children;
}

crow::json::wvalue object_to_json(sc_core::sc_object* obj)
{
    return { { "basename", obj->basename() }, { "name", obj->name() }, { "kind", obj->kind() } };
}

crow::json::wvalue json_object(sc_core::sc_object* obj)
{
    crow::json::wvalue r = object_to_json(obj);
    if (obj->kind() == std::string("tlm_target_socket")) {
        if (dynamic_cast<tlm::tlm_base_target_socket_b<>*>(obj)) {
            // * this object is explorable - treat as memory.

            uint8_t data[8];
            tlm::tlm_generic_payload txn;
            txn.set_command(tlm::TLM_READ_COMMAND);
            txn.set_address(0);
            txn.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
            txn.set_data_length(8);
            txn.set_streaming_width(8);
            txn.set_byte_enable_length(0);
            txn.set_dmi_allowed(false);
            txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            int size = dynamic_cast<tlm::tlm_base_target_socket_b<>*>(obj)->get_base_export()->transport_dbg(txn);

            if (size) {
                r["dbg_port"] = true;
            }
        }
    }

    std::vector<crow::json::wvalue> params = json_cci_params(obj);
    if (params.size()) {
        r["cci_params"] = crow::json::wvalue::list(params);
    }

    std::vector<crow::json::wvalue> co;
    std::vector<crow::json::wvalue> cm;
    std::vector<crow::json::wvalue> cs;

    for (sc_core::sc_object* child : obj->get_child_objects()) {
        co.push_back(object_to_json(child));
        if (child->kind() == std::string("sc_module")) {
            cm.push_back(object_to_json(child));
        }

        if (child->kind() == std::string("tlm_target_socket")) {
            if (dynamic_cast<tlm::tlm_base_target_socket_b<>*>(child)) {
                // * this object is explorable - treat as memory.

                uint8_t data[8];
                tlm::tlm_generic_payload txn;
                txn.set_command(tlm::TLM_READ_COMMAND);
                txn.set_address(0);
                txn.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
                txn.set_data_length(8);
                txn.set_streaming_width(8);
                txn.set_byte_enable_length(0);
                txn.set_dmi_allowed(false);
                txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

                int size = dynamic_cast<tlm::tlm_base_target_socket_b<>*>(child)->get_base_export()->transport_dbg(txn);

                if (size) {
                    cs.push_back(object_to_json(child));
                }
            }
        }
    }
    if (co.size()) {
        r["child_objects"] = crow::json::wvalue::list(co);
    }
    if (cm.size()) {
        r["child_modules"] = crow::json::wvalue::list(cm);
    }
    if (cs.size()) {
        r["child_dbg_ports"] = crow::json::wvalue::list(cs);
    }

    return r;
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::init_monitor()
{
    CROW_ROUTE(m_app, "/")
    ([&]() {
        if (!p_html_doc_template_dir_path.get_value().empty() && !p_html_doc_name.get_value().empty()) {
            crow::mustache::set_global_base(p_html_doc_template_dir_path.get_value());
            auto page = crow::mustache::load(p_html_doc_name.get_value());
            return page.render_string();
        } else {
            std::string ret =
                "API:\n/sc_time\n/pause\n/continue\n/reset\n/object/\n//object/<str>\n/qk_status\n/sc_suspended\n/"
                "transport_dbg/<int>/<str>";
            return ret;
        }
    });
    CROW_ROUTE(m_app, "/sc_time")
    ([&]() {
        crow::json::wvalue r;
        m_sc.run_on_sysc([&] { r["sc_time_stamp"] = sc_core::sc_time_stamp().to_seconds(); });
        return r;
    });
    CROW_ROUTE(m_app, "/pause")
    ([&]() {
        crow::json::wvalue ret;
        execute_qemu_qpm_with_ts_cmd("stop", "STOP", ret);
        m_sc.run_on_sysc([&] {
            sc_core::sc_suspend_all();
            ret["sc_time_stamp"] = sc_core::sc_time_stamp().to_seconds();
        });
        return ret;
    });
    CROW_ROUTE(m_app, "/continue")
    ([&]() {
        crow::json::wvalue ret;
        execute_qemu_qpm_with_ts_cmd("cont", "RESUME", ret);
        m_sc.run_on_sysc([&] {
            sc_core::sc_unsuspend_all();
            ret["sc_time_stamp"] = sc_core::sc_time_stamp().to_seconds();
        });
        return ret;
    });
    CROW_ROUTE(m_app, "/reset")
    ([&]() {
        crow::json::wvalue ret;
        std::string msg = R"({ "execute": "system_reset" })";
        write_to_qmp_socket(msg);
        m_sc.run_on_sysc([&] { ret["sc_time_stamp"] = sc_core::sc_time_stamp().to_seconds(); });
        return ret;
    });
    CROW_ROUTE(m_app, "/object/")
    ([&]() {
        std::vector<crow::json::wvalue> cr;
        m_sc.run_on_sysc([&] {
            for (sc_core::sc_object* child : sc_core::sc_get_top_level_objects()) cr.push_back(json_object(child));
        });
        crow::json::wvalue r = cr;
        return r;
    });
    CROW_ROUTE(m_app, "/object/<str>")
    ([&](std::string name) {
        crow::json::wvalue r;
        m_sc.run_on_sysc([&] {
            auto o = find_sc_obj(nullptr, name, true);
            if (o != nullptr) {
                r = json_object(o);
            } else {
                r["error"] = "Object not found";
            }
        });
        return r;
    });
    CROW_ROUTE(m_app, "/qk_status")
    ([&]() {
        std::vector<crow::json::wvalue> cr;
        for (auto q : m_qks) {
            cr.push_back(crow::json::load(q->get_status_json()));
        }
        crow::json::wvalue r = crow::json::wvalue::list(cr);
        return r;
    });
    CROW_ROUTE(m_app, "/sc_suspended")
    ([&]() {
        crow::json::wvalue r;
        r["sc_suspended"] = (sc_core::sc_get_status() == sc_core::SC_SUSPENDED);
        return r;
    });
    CROW_ROUTE(m_app, "/transport_dbg/<int>/<str>")
    ([&](uint64 addr, std::string name) {
        crow::json::wvalue r;

        auto sc_obj = gs::find_sc_obj(nullptr, name, true);
        auto exp = dynamic_cast<tlm::tlm_base_target_socket_b<>*>(sc_obj);
        if (!exp) {
            r["error"] = "Object not a tlm base target socket";
            return r;
        }
        uint32_t data;
        tlm::tlm_generic_payload txn;
        txn.set_command(tlm::TLM_READ_COMMAND);
        txn.set_address(0);
        txn.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        txn.set_data_length(4);
        txn.set_streaming_width(4);
        txn.set_byte_enable_length(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        int size = exp->get_base_export()->transport_dbg(txn);
        if (size != 4) {
            r["error"] = "Unable to get data";
        } else {
            r["value"] = data;

            r["size"] = size;
        }
        return r;
    });

    m_app_future = m_app.loglevel(crow::LogLevel::Error).port(p_server_port.get_value()).multithreaded().run_async();
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::write_to_qmp_socket(const std::string& msg)
{
    if (!p_use_qmp.get_value()) {
        return;
    }
    try {
        asio::write(m_u_socket, asio::buffer(msg.data(), msg.size()), asio::transfer_all());
    } catch (const asio::system_error& ae) {
        SCP_FATAL(()) << "Couldn't write mesage: " << msg
                      << " on QMP unix domain socket: " << p_qmp_ud_socket_path.get_value() << " Error: " << ae.what();
    }
}

template <unsigned int BUSWIDTH>
std::string monitor<BUSWIDTH>::read_from_qmp_socket()
{
    if (!p_use_qmp.get_value()) {
        return std::string("");
    }
    std::vector<char> data_read;
    data_read.reserve(MAX_ASIO_BUF_LEN);
    asio::socket_base::bytes_readable m_command(true);
    m_u_socket.io_control(m_command);
    size_t readable_bytes = m_command.get();
    data_read.resize(readable_bytes);
    char* data = reinterpret_cast<char*>(&data_read[0]);
    try {
        asio::read(m_u_socket, asio::buffer(data, readable_bytes));
    } catch (const asio::system_error& ae) {
        SCP_FATAL(()) << "Couldn't read from QMP unix domain socket: " << p_qmp_ud_socket_path.get_value()
                      << " Error: " << ae.what();
    }
    auto ret = std::string(data_read.begin(), data_read.end());
    ret.erase(std::remove_if(ret.begin(), ret.end(), [](char c) { return c == '\r' || c == '\n' || c == '\\'; }),
              ret.end());
    return ret;
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::execute_qemu_qpm_with_ts_cmd(const std::string& cmd_name, const std::string& ret_event,
                                                     crow::json::wvalue& resp)
{
    if (!p_use_qmp.get_value()) {
        return;
    }
    std::string r_msg;
    do {
        std::string w_msg = R"({ "execute": ")" + cmd_name + R"(" })";
        write_to_qmp_socket(w_msg);
        r_msg = read_from_qmp_socket();
    } while ((r_msg.find(ret_event) == std::string::npos) && (last_qemu_event.empty() || last_qemu_event != ret_event));

    std::string resp_str = (last_qemu_event != ret_event ? r_msg.substr(0, r_msg.find(ret_event) + ret_event.size() + 2)
                                                         : "");
    if (resp_str.empty()) {
        resp["qemu_time_stamp"] = m_qemu_timestamp_secs;
    } else {
        auto json_obj = crow::json::load(resp_str);
        m_qemu_timestamp_secs = (static_cast<double>(json_obj["timestamp"]["seconds"].u()) +
                                 (static_cast<double>(json_obj["timestamp"]["microseconds"].u()) / 1000'000)) -
                                m_now;
        resp["qemu_time_stamp"] = m_qemu_timestamp_secs;
    }
    last_qemu_event = ret_event;
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
    m_qks = find_sc_objects<gs::tlm_quantumkeeper_multithread>();

    if (p_use_qmp.get_value() && !p_qmp_ud_socket_path.get_value().empty()) {
        try {
            m_u_socket.connect(m_u_endpoint);
            if (!m_u_socket.is_open()) {
                SCP_FATAL(()) << "QMP unix domain socket: " << p_qmp_ud_socket_path.get_value()
                              << " is not opened successfully!";
            }
        } catch (const asio::system_error& ae) {
            SCP_FATAL(()) << "Couldn't connect to QMP unix domain socket: " << p_qmp_ud_socket_path.get_value()
                          << " Error: " << ae.what();
        }
        std::string msg = R"({ "execute": "qmp_capabilities" })";
        write_to_qmp_socket(msg);
    }
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::start_of_simulation()
{
    uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
    m_now = now / 1000'000; // to seconds
}

template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::end_of_simulation()
{
    m_app.stop();
    m_u_socket.close();
}

template class monitor<32>;
template class monitor<64>;
} // namespace gs
typedef gs::monitor<32> monitor;

void module_register() { GSC_MODULE_REGISTER_C(monitor); }
