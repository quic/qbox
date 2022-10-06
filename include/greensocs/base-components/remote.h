/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_REMOTE_H
#define _GREENSOCS_BASE_COMPONENTS_REMOTE_H

#include <cci_configuration>
#include <systemc>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <greensocs/libgsutils.h>
#include <greensocs/libgssync.h>

#include <iomanip>
#include <unistd.h>

#include "rpc/client.h"
#include "rpc/rpc_error.h"
#include "rpc/server.h"
#include "rpc/this_handler.h"
#include "rpc/this_session.h"

#define GS_Process_Serve_Port "GS_Process_Serve_Port"
namespace gs {

/* rpc pass through should pass through ONE forward connection ? */

template <unsigned int BUSWIDTH = 32>
class PassRPC : public sc_core::sc_module
{
    static std::string txn_str(tlm::tlm_generic_payload& trans)
    {
        std::stringstream info;

        const char* cmd = "unknown";
        switch (trans.get_command()) {
        case tlm::TLM_IGNORE_COMMAND:
            cmd = "ignore";
            break;
        case tlm::TLM_WRITE_COMMAND:
            cmd = "write";
            break;
        case tlm::TLM_READ_COMMAND:
            cmd = "read";
            break;
        }

        info << " " << cmd << " to address "
             << "0x" << std::hex << trans.get_address();

        info << " len:" << trans.get_data_length();
        unsigned char* ptr = trans.get_data_ptr();
        info << " returned with data 0x";
        for (int i = trans.get_data_length(); i; i--) {
            info << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(ptr[i - 1]);
        }
        info << " status:" << trans.get_response_status() << " ";
        for (int i = 0; i < tlm::max_num_extensions(); i++) {
            if (trans.get_extension(i)) {
                info << " extn " << i;
            }
        }
        return info.str();
    }

    using str_pairs = std::vector<std::pair<std::string, std::string>>;

    struct tlm_generic_payload_rpc {
        sc_dt::uint64 m_address;
        int m_command;
        unsigned int m_length;
        int m_response_status;
        bool m_dmi;
        unsigned int m_byte_enable_length;
        unsigned int m_streaming_width;
        int m_gp_option;

        RPCLIB_MSGPACK::type::raw_ref m_data;
        RPCLIB_MSGPACK::type::raw_ref m_byte_enable;
        // extensions will not be carried
        MSGPACK_DEFINE_ARRAY(m_address, m_command, m_length, m_response_status, m_dmi,
                             m_byte_enable_length, m_streaming_width, m_gp_option, m_data,
                             m_byte_enable);

        void from_tlm(tlm::tlm_generic_payload& other)
        {
            m_command = other.get_command();
            m_address = other.get_address();
            m_length = other.get_data_length();
            m_response_status = other.get_response_status();
            m_byte_enable_length = other.get_byte_enable_length();
            m_streaming_width = other.get_streaming_width();
            m_gp_option = other.get_gp_option();
            m_dmi = other.is_dmi_allowed();
            m_data.ptr = reinterpret_cast<const char*>(other.get_data_ptr());
            m_data.size = m_length;
            m_byte_enable.ptr = reinterpret_cast<const char*>(other.get_byte_enable_ptr());
            m_byte_enable.size = m_byte_enable_length;
        }
        void deep_copy_to_tlm(tlm::tlm_generic_payload& other)
        {
            other.set_command((tlm::tlm_command)(m_command));
            other.set_address(m_address);
            other.set_data_length(m_length);
            other.set_response_status((tlm::tlm_response_status)(m_response_status));
            other.set_byte_enable_length(m_byte_enable_length);
            other.set_streaming_width(m_streaming_width);
            other.set_gp_option((tlm::tlm_gp_option)(m_gp_option));
            other.set_dmi_allowed(m_dmi);
            other.set_data_ptr(
                const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(m_data.ptr)));
            if (!m_byte_enable_length) {
                other.set_byte_enable_ptr(nullptr);
            } else {
                other.set_byte_enable_ptr(const_cast<unsigned char*>(
                    reinterpret_cast<const unsigned char*>(m_byte_enable.ptr)));
            }
        }

        void update_to_tlm(tlm::tlm_generic_payload& other)
        {
            tlm::tlm_generic_payload tmp; // make use of TLM's built in update
            tmp.set_data_ptr(
                const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(m_data.ptr)));
            if (!m_byte_enable_length) {
                other.set_byte_enable_ptr(nullptr);
            } else {
                tmp.set_byte_enable_ptr(const_cast<unsigned char*>(
                    reinterpret_cast<const unsigned char*>(m_byte_enable.ptr)));
            }
            tmp.set_byte_enable_length(m_byte_enable_length);
            tmp.set_response_status((tlm::tlm_response_status)m_response_status);
            tmp.set_dmi_allowed(m_dmi);
            other.update_original_from(tmp, m_byte_enable_length > 0);
        }
    };

    cci::cci_broker_handle m_broker;

    /* Alias from - to */
    void alias_preset_param(std::string a, std::string b, bool required = false)
    {
        if (m_broker.has_preset_value(a)) {
            m_broker.set_preset_cci_value(b, m_broker.get_preset_cci_value(a));
            m_broker.lock_preset_value(a);
            m_broker.ignore_unconsumed_preset_values(
                [a](const std::pair<std::string, cci::cci_value>& iv) -> bool {
                    return iv.first == a;
                });
        } else {
            if (required) {
                SC_REPORT_FATAL("PassRPC ", (std::string(name()) + " Can't find " + a).c_str());
            }
        }
    }

    template <typename MOD>
    class multi_passthrough_initiator_socket_spying
        : public tlm_utils::multi_passthrough_initiator_socket<MOD, BUSWIDTH>
    {
        using typename tlm_utils::multi_passthrough_initiator_socket<
            MOD, BUSWIDTH>::base_target_socket_type;

        const std::function<void(std::string)> register_cb;

    public:
        multi_passthrough_initiator_socket_spying(const char* name,
                                                  const std::function<void(std::string)>& f)
            : tlm_utils::multi_passthrough_initiator_socket<
                  MOD, BUSWIDTH>::multi_passthrough_initiator_socket(name)
            , register_cb(f)
        {
        }

        void bind(base_target_socket_type& socket)
        {
            tlm_utils::multi_passthrough_initiator_socket<MOD, BUSWIDTH>::bind(socket);
            register_cb(socket.get_base_export().name());
        }
    };

    /* NB use the EXPORT name, so as not to be hassled by the _port_0*/
    std::string nameFromSocket(std::string s) { return s; }

    void remote_register_boundto(std::string s)
    {
        std::string pfx = nameFromSocket(s);
        std::string n = std::string(name());
        std::string parent = std::string(n).substr(0, n.find_last_of("."));
        if (pfx.find(parent) == 0) {
            pfx = pfx.substr(parent.length() + 1);
        }
        str_pairs vals;
        std::vector<std::string> todo = { "address", "size", "relative_addresses" };
        for (auto i : todo) {
            if (m_broker.has_preset_value(s + "." + i)) {
                vals.push_back(std::make_pair(
                    pfx + "." + i, "target_socket_" + std::to_string(targets_bound) + "." + i));
                std::cout << "sending alias " << pfx + "." + i << " target_soceket_"
                          << targets_bound << "." << i << "\n";
            }
        }
        client->call("cci_alias", vals);
        targets_bound++;
    }

public:
    // NB there is a 'feature' in multi passthrough sockets, the 'name' is always returned as the
    // name of the socket itself, in our case "target_socket_0". This means that address lookup will
    // only work for the FIRST such socket, all others will require a 'pass' or should be driven
    // from models that dont use the CCI address map info.
    tlm_utils::multi_passthrough_target_socket<PassRPC<BUSWIDTH>, BUSWIDTH,
                                               tlm::tlm_base_protocol_types, 0,
                                               sc_core::SC_ZERO_OR_MORE_BOUND>
        target_socket;
    multi_passthrough_initiator_socket_spying<PassRPC<BUSWIDTH>> initiator_socket;

    cci::cci_param<int> p_cport;
    cci::cci_param<int> p_sport;
    cci::cci_param<std::string> p_exec_path;
    cci::cci_param<std::string> p_sync_policy;

private:
    rpc::client* client = nullptr;
    rpc::server* server = nullptr;
    int m_child_pid = 0;

    bool m_beoe = false;
    bool m_start_of_sim = false;
    int targets_bound = 0;

    std::shared_ptr<gs::tlm_quantumkeeper_extended> qk;

    /* b_transport interface */
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        tlm_generic_payload_rpc t;
        double time = sc_core::sc_time_stamp().to_seconds();

        t.from_tlm(trans);
        std::cout << name() << " b_transport socket ID " << id << " From_tlm " << txn_str(trans)
                  << "\n";
        tlm_generic_payload_rpc r = client->call("b_tspt", id, t, time)
                                        .template as<tlm_generic_payload_rpc>();
        r.update_to_tlm(trans);
        std::cout << name() << " update_to_tlm " << txn_str(trans) << "\n";
    }
    tlm_generic_payload_rpc b_transport_rpc(int id, tlm_generic_payload_rpc t, double time)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);
        std::cout << name() << " deep copy to_tlm " << txn_str(trans) << "\n";
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        qk->run_on_systemc([&] { initiator_socket[id]->b_transport(trans, delay); });
        t.from_tlm(trans);
        std::cout << name() << " from_tlm " << txn_str(trans) << "\n";
        return t;
    }

    /* Debug transport interface */
    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        tlm_generic_payload_rpc t;

        t.from_tlm(trans);
        std::cout << name() << " dbg_transport socket ID " << id << " From_tlm " << txn_str(trans)
                  << "\n";
        tlm_generic_payload_rpc r = client->call("dbg_tspt", id, t)
                                        .template as<tlm_generic_payload_rpc>();
        r.update_to_tlm(trans);
        // this is not entirely accurate, but see below
        return trans.get_response_status() == tlm::TLM_OK_RESPONSE ? trans.get_data_length() : 0;
    }
    tlm_generic_payload_rpc transport_dbg_rpc(int id, tlm_generic_payload_rpc t)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);
        std::cout << name() << " deep copy to_tlm " << txn_str(trans) << "\n";
        int ret_len;
        qk->run_on_systemc([&] { ret_len = initiator_socket[id]->transport_dbg(trans); });
        t.from_tlm(trans);

        if (!(trans.get_data_length() == ret_len ||
              trans.get_response_status() != tlm::TLM_OK_RESPONSE)) {
            SC_REPORT_WARNING("PassRPC",
                              "debug transaction not able to access required length of data.");
        }
        return t;
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        //        if (p_verbose) {
        std::stringstream info;
        info << " " << name() << " get_direct_mem_ptr to address "
             << "0x" << std::hex << trans.get_address();
        SC_REPORT_INFO("PassRPC", info.str().c_str());
        //        }
        //        return initiator_socket->get_direct_mem_ptr(trans, dmi_data);
    }

    /* Invalidate DMI Interface */
    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        //        if (p_verbose) {
        std::stringstream info;
        info << " " << name() << " invalidate_direct_mem_ptr "
             << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;
        SC_REPORT_INFO("PassRPC", info.str().c_str());
        //        }
        client->call("dmi_inv", start, end);
    }
    void invalidate_direct_mem_ptr_rpc(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        //        if (p_verbose) {
        std::stringstream info;
        info << " " << name() << " invalidate_direct_mem_ptr "
             << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;
        SC_REPORT_INFO("PassRPC", info.str().c_str());
        //        }

        qk->run_on_systemc([&] {
            for (int i = 0; i < target_socket.size(); i++) {
                target_socket[i]->invalidate_direct_mem_ptr(start, end);
            }
        });
    }

    str_pairs get_cci_db()
    {
        std::string n = std::string(name());
        std::string parent = std::string(n).substr(0, n.find_last_of("."));

        str_pairs ret;
        for (auto p : m_broker.get_unconsumed_preset_values()) {
            std::string k = p.first;
            if (k.find(parent) == 0) {
                k = k.substr(parent.length() + 1);
            }
            ret.push_back(std::make_pair(k, p.second.to_json()));
        }
        return ret;
    }

    void set_cci_db(str_pairs db)
    {
        std::string n = std::string(name());
        std::string parent = std::string(n).substr(0, n.find_last_of("."));

        for (auto p : db) {
            m_broker.set_preset_cci_value(parent + "." + p.first,
                                          cci::cci_value::from_json(p.second));
            std::cout << "Setting " << parent + ".:" + p.first << "\n";
        }
    }

    void set_cci_alias(str_pairs db)
    {
        std::string n = std::string(name());
        std::string parent = std::string(n).substr(0, n.find_last_of("."));
        for (auto p : db) {
            alias_preset_param(parent + "." + p.first, n + "." + p.second, true);
            std::cout << "Aliasing from " << parent + "." + p.first << " to " << n + "." + p.second
                      << "\n";
        }
    }

public:
    PassRPC(const sc_core::sc_module_name& nm, std::string exec_path = "", int port = 0)
        : sc_core::sc_module(nm)
        , m_broker(cci::cci_get_broker())
        , initiator_socket("initiator_socket",
                           [&](std::string s) -> void { remote_register_boundto(s); })
        , target_socket("target_socket_0")
        , p_cport("client_port", port,
                  "The port that should be used to connect this client to the "
                  "remote server")
        , p_sport("server_port", 0, "The port that should be used to server on")
        , p_exec_path("exec_path", exec_path,
                      "The path to the executable that should be started by "
                      "the bridge")
        , p_sync_policy("sync_policy", "multithread-unconstrained", "Sync policy for the remote")
    {
        // always serve on a new port.
        server = new rpc::server(p_sport);
        server->suppress_exceptions(true);
        p_sport = server->port();
        assert(p_sport > 0);

        if (p_cport.get_value() == 0 && m_broker.has_preset_value(GS_Process_Serve_Port)) {
            p_cport = gs::cci_get<int>(GS_Process_Serve_Port);
        }

        if (p_cport) {
            std::cout << "Connecting client on port " << p_cport << "\n";
            client = new rpc::client("localhost", p_cport);
            client->set_timeout(2000);
            set_cci_db(client->call("reg", (int)p_sport).as<str_pairs>());
        }

        // other end contacted us, connect to their port
        // and return back the cci database
        server->bind("reg", [&](int port) {
            std::cout << "reg " << name() << " 1\n";
            assert(p_cport == 0 && client == nullptr);
            p_cport = port;
            client = new rpc::client("localhost", p_cport);
            client->set_timeout(2000);
            return get_cci_db();
        });

        // would it be better to have a 'remote cci broker' that connected back,

        server->bind("cci_alias", [&](str_pairs db) { set_cci_alias(db); });

        server->bind("beoe", [&]() {
            std::cout << "BEOE " << name() << " 1\n";
            m_beoe = true;
        });
        server->bind("start", [&]() {
            std::cout << name() << " recieved SOS\n";
            m_start_of_sim = true;
        });
        server->bind("b_tspt", [&](int id, tlm_generic_payload_rpc txn, double time) {
            return PassRPC::b_transport_rpc(id, txn, time);
        });

        server->bind("dbg_tspt", [&](int id, tlm_generic_payload_rpc txn, double time) {
            return PassRPC::transport_dbg_rpc(id, txn);
        });

        server->bind("dmi_inv", [&](uint64_t start, uint64_t end) {
            return PassRPC::invalidate_direct_mem_ptr_rpc(start, end);
        });

        server->bind("exit", [&](int i) {
            qk->stop();
            std::cout << "exit " << name() << "\n\n";
            rpc::this_session().post_exit();
        });

        target_socket.register_b_transport(this, &PassRPC::b_transport);
        target_socket.register_transport_dbg(this, &PassRPC::transport_dbg);
        target_socket.register_get_direct_mem_ptr(this, &PassRPC::get_direct_mem_ptr);

        qk = tlm_quantumkeeper_factory(p_sync_policy);
        qk->start();
        qk->reset();
        server->async_run(10);

        if (!exec_path.empty()) {
            std::cout << "Forking remote " << exec_path << "\n";
            m_child_pid = fork();
            if (m_child_pid == 0) {
                char conf_arg[100];
                sprintf(conf_arg, "%s=%d", GS_Process_Serve_Port, (int)p_sport);
                execlp(exec_path.c_str(), exec_path.c_str(), "-p", conf_arg, nullptr);
                // execlp("lldb", "lldb", "--", exec_path.c_str(), exec_path.c_str(), "-p",
                // conf_arg, nullptr);
                SC_REPORT_FATAL("PassRPC", "Unable to find executable for remote");
            }
        }

        // Make sure by now the client is connected so we can send/recieve.
        for (int i = 0; i < 10 && !p_cport; i++)
            sleep(1);
        assert(p_cport);
    }
    PassRPC(const sc_core::sc_module_name& nm, int port)
        : PassRPC(nm, "", port){}; // convenience constructor

    void before_end_of_elaboration()
    {
        client->call("beoe");
        for (int i = 0; i < 10 && !m_beoe; i++)
            sleep(1);
        assert(m_beoe);
    }
    void start_of_simulation()
    {
        client->call("start");
        for (int i = 0; i < 10 && !m_start_of_sim; i++)
            sleep(1);
        assert(m_start_of_sim);
    }
    PassRPC() = delete;
    PassRPC(const PassRPC&) = delete;
    ~PassRPC()
    {
        qk->stop();
        std::cout << "EXIT " << name() << "\n\n";
        if (client) {
            client->call("exit", 0);
            sleep(1);
        }
    }

    void end_of_simulation()
    {
        qk->stop();
        if (client) {
            client->call("exit", 0);
            sleep(1);
        }
    }
};

} // namespace gs
#endif
