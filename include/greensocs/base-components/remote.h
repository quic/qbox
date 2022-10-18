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
#include <tlm>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

#include <greensocs/libgsutils.h>
#include <greensocs/libgssync.h>
#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <greensocs/gsutils/ports/target-signal-socket.h>
#include <iomanip>
#include <unistd.h>

#include "memory_services.h"

#include "rpc/client.h"
#include "rpc/rpc_error.h"
#include "rpc/server.h"
#include "rpc/this_handler.h"
#include "rpc/this_session.h"

#define GS_Process_Serve_Port "GS_Process_Serve_Port"
namespace gs {

/* rpc pass through should pass through ONE forward connection ? */

template <unsigned int BUSWIDTH = 32, unsigned int SIGNALS=1>
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

    /* Handle local DMI cache */
    struct tlm_dmi_cache_entry {
        std::string m_shmem_fn;
        tlm::tlm_dmi dmi;
        tlm_dmi_cache_entry(std::string n, tlm::tlm_dmi d): dmi(d), m_shmem_fn(n) {}
        ~tlm_dmi_cache_entry()
        {
            std::cout << "HERE!!!\n\n\n";
            //            munmap(dmi.get_dmi_ptr(), (dmi.get_end_address() -
            //            dmi.get_start_address()) + 1);
        }
    };
    using tlm_dmi_cache = std::unique_ptr<tlm_dmi_cache_entry>;
    std::vector<tlm_dmi_cache> dmi_cache;
    tlm::tlm_dmi* in_cache(uint64_t address)
    {
        auto it = find_if(dmi_cache.begin(), dmi_cache.end(), [&](tlm_dmi_cache const &t) {
            return (t->dmi.get_start_address() <= address) && (t->dmi.get_end_address() >= address);
        });
        if (it != dmi_cache.end()) {
            return &((*it)->dmi);
        } else {
            return nullptr;
        }
    }
    tlm::tlm_dmi* in_cache(std::string name)
    {
        auto it = find_if(dmi_cache.begin(), dmi_cache.end(),
                          [&](tlm_dmi_cache const &t) { return (t->m_shmem_fn == name); });
        if (it != dmi_cache.end()) {
            return &((*it)->dmi);
        } else {
            return nullptr;
        }
    }
    void cache_clean(uint64_t start, uint64_t end)
    {
        dmi_cache.erase(remove_if(dmi_cache.begin(), dmi_cache.end(), [&](tlm_dmi_cache const &t) {
            return (start <= t->dmi.get_start_address() && end >= t->dmi.get_start_address()) ||
                   (start <= t->dmi.get_end_address() && end >= t->dmi.get_end_address());
        }));
    }
    /* RPC structure for TLM_DMI */
    struct tlm_dmi_rpc {
        std::string m_shmem_fn;

        uint64_t m_dmi_start_address;
        uint64_t m_dmi_end_address;
        int m_dmi_access; /*tlm::tlm_dmi::dmi_access_e */
        double m_dmi_read_latency;
        double m_dmi_write_latency;

        std::vector<tlm_dmi_cache> dmi_cache;

        MSGPACK_DEFINE_ARRAY(m_shmem_fn, m_dmi_start_address, m_dmi_end_address, m_dmi_access,
                             m_dmi_read_latency, m_dmi_write_latency);

        void from_tlm(tlm::tlm_dmi& other, std::string fn)
        {
            m_shmem_fn = fn;

            m_dmi_start_address = other.get_start_address();
            m_dmi_end_address = other.get_end_address();
            m_dmi_access = other.get_granted_access();
            m_dmi_read_latency = other.get_read_latency().to_seconds();
            m_dmi_write_latency = other.get_write_latency().to_seconds();
        }

        void to_tlm(tlm::tlm_dmi& other)
        {
            if (m_shmem_fn.empty())
                return;

            other.set_dmi_ptr(MemoryServices::get().map_mem_join(
                m_shmem_fn.c_str(), (m_dmi_end_address - m_dmi_start_address) + 1));

            other.set_start_address(m_dmi_start_address);
            other.set_end_address(m_dmi_end_address);
            other.set_granted_access((tlm::tlm_dmi::dmi_access_e)m_dmi_access);
            other.set_read_latency(sc_core::sc_time(m_dmi_read_latency, sc_core::SC_SEC));
            other.set_write_latency(sc_core::sc_time(m_dmi_write_latency, sc_core::SC_SEC));
        }
    };

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
        : public tlm_utils::multi_passthrough_initiator_socket<
              MOD, BUSWIDTH, tlm::tlm_base_protocol_types, 0, sc_core::SC_ZERO_OR_MORE_BOUND>
    {
        using typename tlm_utils::multi_passthrough_initiator_socket<
            MOD, BUSWIDTH, tlm::tlm_base_protocol_types, 0,
            sc_core::SC_ZERO_OR_MORE_BOUND>::base_target_socket_type;

        const std::function<void(std::string)> register_cb;

    public:
        multi_passthrough_initiator_socket_spying(const char* name,
                                                  const std::function<void(std::string)>& f)
            : tlm_utils::multi_passthrough_initiator_socket<
                  MOD, BUSWIDTH, tlm::tlm_base_protocol_types, 0,
                  sc_core::SC_ZERO_OR_MORE_BOUND>::multi_passthrough_initiator_socket(name)
            , register_cb(f)
        {
        }

        void bind(base_target_socket_type& socket)
        {
            tlm_utils::multi_passthrough_initiator_socket<
                MOD, BUSWIDTH, tlm::tlm_base_protocol_types, 0,
                sc_core::SC_ZERO_OR_MORE_BOUND>::bind(socket);
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

    sc_core::sc_vector<InitiatorSignalSocket<bool>> initiator_signal_sockets;
    sc_core::sc_vector<TargetSignalSocket<bool>> target_signal_sockets;

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

        uint64_t addr = trans.get_address();
        // If we have a locally cached DMI, use it!
        tlm::tlm_dmi* c = in_cache(addr);
        if (c) {
            uint64_t len = trans.get_data_length();
            if (addr >= c->get_start_address() &&
                addr + len <= c->get_end_address()) {
                switch (trans.get_command()) {
                case tlm::TLM_IGNORE_COMMAND:
                    break;
                case tlm::TLM_WRITE_COMMAND:
                    memcpy(c->get_dmi_ptr() +
                               (addr - c->get_start_address()),
                           trans.get_data_ptr(), len);
                    break;
                case tlm::TLM_READ_COMMAND:
                    memcpy(trans.get_data_ptr(),
                           c->get_dmi_ptr() +
                               (addr - c->get_start_address()),
                           len);
                    break;
                }
                trans.set_dmi_allowed(true);
                trans.set_response_status(tlm::TLM_OK_RESPONSE);
                return;
            }
        }

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
        tlm::tlm_dmi* c;
        //        if (p_verbose) {
        std::stringstream info;
        info << " " << name() << " get_direct_mem_ptr to address "
             << "0x" << std::hex << trans.get_address();
        SC_REPORT_INFO("PassRPC", info.str().c_str());
        //        }

        c = in_cache(trans.get_address());
        if (c) {
            std::cout << "In Cache "<<std::hex<<c->get_start_address()<<" - "<<std::hex<<c->get_end_address()<<"\n";
            dmi_data = *c;
            return !(dmi_data.is_none_allowed());
        }

        tlm_generic_payload_rpc t;
        t.from_tlm(trans);
        std::cout << name() << " DMI socket ID " << id << " From_tlm " << txn_str(trans) << "\n";
        tlm_dmi_rpc r = client->call("dmi_req", id, t).template as<tlm_dmi_rpc>();
        if (r.m_shmem_fn.empty())
            return false;
        c = in_cache(r.m_shmem_fn);
        if (c) {
            dmi_data = *c;
        } else {
            r.to_tlm(dmi_data);
std::cout << "Adding "<<r.m_shmem_fn<<" " << dmi_data.get_start_address()<<" to cache\n";
            dmi_cache.push_back(std::make_unique<tlm_dmi_cache_entry>(r.m_shmem_fn, dmi_data));
        }
        return !(dmi_data.is_none_allowed());
    }

    tlm_dmi_rpc get_direct_mem_ptr_rpc(int id, tlm_generic_payload_rpc t)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);

        //        if (p_verbose) {
        std::stringstream info;
        info << " " << name() << " get_direct_mem_ptr " << txn_str(trans) << "\n";
        SC_REPORT_INFO("PassRPC", info.str().c_str());
        //        }

        tlm::tlm_dmi dmi_data;
        tlm_dmi_rpc ret;
        if (initiator_socket->get_direct_mem_ptr(trans, dmi_data)) {
            ShmemIDExtension* ext = trans.get_extension<ShmemIDExtension>();
            if (!ext)
                return ret;
            ret.from_tlm(dmi_data, *ext);
        }
        return ret;
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
            // remove from local DMI cache inside SystemC to prevent race conditions
            cache_clean(start, end);
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
        , initiator_signal_sockets("initiator_signal_socket", SIGNALS, [this](const char* n, int i) { return new InitiatorSignalSocket<bool>(n); })
        , target_signal_sockets("target_signal_socket", SIGNALS, [this](const char* n, int i) { return new TargetSignalSocket<bool>(n); })
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

        server->bind("dbg_tspt", [&](int id, tlm_generic_payload_rpc txn) {
            return PassRPC::transport_dbg_rpc(id, txn);
        });

        server->bind("dmi_inv", [&](uint64_t start, uint64_t end) {
            return PassRPC::invalidate_direct_mem_ptr_rpc(start, end);
        });

        server->bind("dmi_req", [&](int id, tlm_generic_payload_rpc txn) {
            return PassRPC::get_direct_mem_ptr_rpc(id, txn);
        });

        server->bind("exit", [&](int i) {
            qk->stop();
            std::cout << "exit " << name() << "\n\n";
            rpc::this_session().post_exit();
        });

        server->bind("signal", [&](int i, bool v) {
            initiator_signal_sockets[i]->write(v);
        });

        target_socket.register_b_transport(this, &PassRPC::b_transport);
        target_socket.register_transport_dbg(this, &PassRPC::transport_dbg);
        target_socket.register_get_direct_mem_ptr(this, &PassRPC::get_direct_mem_ptr);

        for (int i=0; i< SIGNALS; i++ ) {
        target_signal_sockets[i].register_value_changed_cb([&,i](bool value) {
            client->call("signal", i, value);
        });
        }

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
        dmi_cache.clear();
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
