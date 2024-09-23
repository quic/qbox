/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define SC_ALLOW_DEPRECATED_IEEE_API

#include <monitor.h>
#include <module_factory_registery.h>
// #include <pybind11/stl.h>
// #include <pybind11/embed.h>
// #include <pybind11/functional.h>
// #include <pybind11/operators.h>
// #include <string>
#include <vector>
#include <functional>
#include <exception>
// #include <stdexcept>
// #include <cstdint>
// #include <exception>

#include <cciutils.h>

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

    //
    //        return crow::json::wvalue::list(r);
    //    }
    return r;
}
template <unsigned int BUSWIDTH>
void monitor<BUSWIDTH>::init_monitor()
{
    CROW_ROUTE(m_app, "/")
    ([&]() {
        std::string ret = m_html;
        return ret;
    });
    CROW_ROUTE(m_app, "/sc_time")
    ([&]() {
        crow::json::wvalue r;
        m_sc.run_on_sysc([&] { r["sc_time_stamp"] = sc_core::sc_time_stamp().to_string(); });
        return r;
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
