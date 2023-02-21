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
#include <scp/report.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

#include <greensocs/libgsutils.h>
#include <greensocs/gsutils/uutils.h>
#include <greensocs/libgssync.h>
#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <greensocs/gsutils/ports/target-signal-socket.h>
#include <iomanip>
#include <unistd.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>

#include "memory_services.h"

#include "rpc/client.h"
#include "rpc/rpc_error.h"
#include "rpc/server.h"
#include "rpc/this_handler.h"
#include "rpc/this_session.h"

#define GS_Process_Server_Port "GS_Process_Server_Port"

namespace gs {

// #define DMICACHE switchthis on - then you need a mutex
/* rpc pass through should pass through ONE forward connection ? */

template <unsigned int TLMPORTS = 1, unsigned int SIGNALS = 1, unsigned int BUSWIDTH = 32>
class PassRPC : public sc_core::sc_module
{
    SCP_LOGGER();
    using MOD = PassRPC<TLMPORTS, SIGNALS, BUSWIDTH>;

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
        if (trans.is_dmi_allowed())
            info << " DMI OK ";
        for (int i = 0; i < tlm::max_num_extensions(); i++) {
            if (trans.get_extension(i)) {
                info << " extn " << i;
            }
        }
        return info.str();
    }

    using str_pairs = std::vector<std::pair<std::string, std::string>>;
#ifdef DMICACHE
    /* Handle local DMI cache */

    std::map<uint64_t, tlm::tlm_dmi> m_dmi_cache;
    tlm::tlm_dmi* in_cache(uint64_t address)
    {
        if (m_dmi_cache.size() > 0) {
            auto it = m_dmi_cache.upper_bound(address);
            if (it != m_dmi_cache.begin()) {
                it = std::prev(it);
                if ((address >= it->second.get_start_address()) &&
                    (address <= it->second.get_end_address())) {
                    return &(it->second);
                }
            }
        }
        return nullptr;
    }
    void cache_clean(uint64_t start, uint64_t end)
    {
        auto it = m_dmi_cache.upper_bound(start);

        if (it != m_dmi_cache.begin()) {
            /*
             * Start with the preceding region, as it may already cross the
             * range we must invalidate.
             */
            it--;
        }

        while (it != m_dmi_cache.end()) {
            tlm::tlm_dmi& r = it->second;

            if (r.get_start_address() > end) {
                /* We've got out of the invalidation range */
                break;
            }

            if (r.get_end_address() < start) {
                /* We are not in yet */
                it++;
                continue;
            }
            it = m_dmi_cache.erase(it);
        }
    }
#endif
    /* RPC structure for TLM_DMI */
    struct tlm_dmi_rpc {
        std::string m_shmem_fn;
        size_t m_shmem_size;
        uint64_t m_shmem_offset;

        uint64_t m_dmi_start_address;
        uint64_t m_dmi_end_address;
        int m_dmi_access; /*tlm::tlm_dmi::dmi_access_e */
        double m_dmi_read_latency;
        double m_dmi_write_latency;

        MSGPACK_DEFINE_ARRAY(m_shmem_fn, m_shmem_size, m_shmem_offset, m_dmi_start_address,
                             m_dmi_end_address, m_dmi_access, m_dmi_read_latency,
                             m_dmi_write_latency);

        void from_tlm(tlm::tlm_dmi& other, ShmemIDExtension* shm)
        {
            m_shmem_fn = shm->m_memid;
            m_shmem_size = shm->m_size;
            //            SCP_DEBUG(()) << "DMI from tlm Size "<<m_shmem_size;

            m_shmem_offset = (uint64_t)(other.get_dmi_ptr()) - shm->m_mapped_addr;
            m_dmi_start_address = other.get_start_address();
            m_dmi_end_address = other.get_end_address();
            m_dmi_access = other.get_granted_access();
            m_dmi_read_latency = other.get_read_latency().to_seconds();
            m_dmi_write_latency = other.get_write_latency().to_seconds();
        }

        void to_tlm(tlm::tlm_dmi& other)
        {
            if (m_shmem_size == 0)
                return;

            //            SCP_DEBUG(()) << "DMI to_tlm Size "<<m_shmem_size;

            other.set_dmi_ptr(m_shmem_offset +
                              MemoryServices::get().map_mem_join(m_shmem_fn.c_str(), m_shmem_size));
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

        double m_sc_time;
        double m_quantum_time;

        RPCLIB_MSGPACK::type::raw_ref m_data;
        RPCLIB_MSGPACK::type::raw_ref m_byte_enable;
        // extensions will not be carried
        MSGPACK_DEFINE_ARRAY(m_address, m_command, m_length, m_response_status, m_dmi,
                             m_byte_enable_length, m_streaming_width, m_gp_option, m_sc_time,
                             m_quantum_time, m_data, m_byte_enable);

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

    class initiator_socket_spying
        : public tlm_utils::simple_initiator_socket_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                      sc_core::SC_ZERO_OR_MORE_BOUND>
    {
        using socket_type = tlm_utils::simple_initiator_socket_b<
            MOD, BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;
        using typename socket_type::
            base_target_socket_type; // tlm_utils::simple_initiator_socket<MOD,
                                     // BUSWIDTH>::base_target_socket_type;

        const std::function<void(std::string)> register_cb;

    public:
        initiator_socket_spying(const char* name, const std::function<void(std::string)>& f)
            : socket_type::simple_initiator_socket_b(name), register_cb(f)
        {
        }

        void bind(base_target_socket_type& socket)
        {
            socket_type::bind(socket);
            register_cb(socket.get_base_export().name());
        }

        // hierarchial binding
        void bind(tlm::tlm_initiator_socket<BUSWIDTH>& socket)
        {
            socket_type::bind(socket);
            register_cb(socket.get_base_port().name());
        }
    };

    /* NB use the EXPORT name, so as not to be hassled by the _port_0*/
    std::string nameFromSocket(std::string s) { return s; }

    void remote_register_boundto(std::string s)
    {
        str_pairs vals;
        std::vector<std::string> todo = { "address", "size", "relative_addresses" };
        for (auto i : todo) {
            std::string name = s + "." + i;
            if (m_broker.has_preset_value(name)) {
                m_broker.ignore_unconsumed_preset_values(
                    [name](const std::pair<std::string, cci::cci_value>& iv) -> bool {
                        return iv.first == name;
                    });

                vals.push_back(
                    std::make_pair("@target_socket_" + std::to_string(targets_bound) + "." + i,
                                   m_broker.get_preset_cci_value(name).to_json()));
                SCP_DEBUG(()) << "sending  "
                              << "@target_socket_" + std::to_string(targets_bound) + "." + i
                              << " to " << m_broker.get_preset_cci_value(name).to_json();
            }
        }
        client->call("cci_db", vals);
        targets_bound++;
    }

public:
    // NB there is a 'feature' in multi passthrough sockets, the 'name' is always returned as the
    // name of the socket itself, in our case "target_socket_0". This means that address lookup will
    // only work for the FIRST such socket, all others will require a 'pass' or should be driven
    // from models that dont use the CCI address map info.
    // We can fix this using a template and vector of sockets....
    using tlm_target_socket = tlm_utils::simple_target_socket_tagged_b<
        MOD, BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;

    sc_core::sc_vector<tlm_target_socket> target_sockets;
    sc_core::sc_vector<initiator_socket_spying> initiator_sockets;

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
    ProcAliveHandler pahandler;
    std::condition_variable is_client_connected;
    std::condition_variable is_sc_status_set;
    std::mutex client_conncted_mut;
    std::mutex sc_status_mut;

    sc_core::sc_status m_remote_status = static_cast<sc_core::sc_status>(0);

    int targets_bound = 0;

    std::shared_ptr<gs::tlm_quantumkeeper_extended> m_qk;
    gs::RunOnSysC m_sc;

    /* b_transport interface */
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        tlm_generic_payload_rpc t;
        double time = sc_core::sc_time_stamp().to_seconds();

        uint64_t addr = trans.get_address();
        // If we have a locally cached DMI, use it!
#ifdef DMICACHE
        auto c = in_cache(addr);
        if (c) {
            uint64_t len = trans.get_data_length();
            if (addr >= c->get_start_address() && addr + len <= c->get_end_address()) {
                switch (trans.get_command()) {
                case tlm::TLM_IGNORE_COMMAND:
                    break;
                case tlm::TLM_WRITE_COMMAND:
                    memcpy(c->get_dmi_ptr() + (addr - c->get_start_address()), trans.get_data_ptr(),
                           len);
                    break;
                case tlm::TLM_READ_COMMAND:
                    memcpy(trans.get_data_ptr(), c->get_dmi_ptr() + (addr - c->get_start_address()),
                           len);
                    break;
                }
                trans.set_dmi_allowed(true);
                trans.set_response_status(tlm::TLM_OK_RESPONSE);
                return;
            }
        }
#endif
        t.from_tlm(trans);
        t.m_quantum_time = delay.to_seconds();
        t.m_sc_time = sc_core::sc_time_stamp().to_seconds();

        //        SCP_DEBUG(()) << name() << " b_transport socket ID " << id << " From_tlm " <<
        //        txn_str(trans); SCP_DEBUG(()) << getpid() <<" IS THE b_ RPC PID " <<
        //        std::this_thread::get_id() <<" is the thread ID";
        tlm_generic_payload_rpc r = client->call("b_tspt", id, t)
                                        .template as<tlm_generic_payload_rpc>();
        r.update_to_tlm(trans);
        delay = sc_core::sc_time(t.m_quantum_time, sc_core::SC_SEC);
        sc_core::sc_time other_time = sc_core::sc_time(t.m_sc_time, sc_core::SC_SEC);
        //        m_qk->set(other_time+delay);
        //        m_qk->sync();
        //        SCP_DEBUG(()) << name() << " update_to_tlm " << txn_str(trans);
        //        SCP_DEBUG(()) << name() << " b_transport socket ID " << id << " returned " <<
        //        txn_str(trans);
    }
    tlm_generic_payload_rpc b_transport_rpc(int id, tlm_generic_payload_rpc t)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);
        //       SCP_DEBUG(()) << name() << " deep copy to_tlm " << txn_str(trans);
        sc_core::sc_time delay = sc_core::sc_time(t.m_quantum_time, sc_core::SC_SEC);
        sc_core::sc_time other_time = sc_core::sc_time(t.m_sc_time, sc_core::SC_SEC);

        //       SCP_DEBUG(()) << "Here"<<m_qk->time_to_sync();
        //        m_qk->sync();
        //        SCP_DEBUG(()) << "THere";
        //        SCP_DEBUG(()) << getpid() <<" IS THE rpc RPC PID " <<
        //        std::this_thread::get_id() <<" is the thread ID";
        m_sc.run_on_sysc([&] {
            //             SCP_DEBUG(()) <<"gere "<<id;
            //        SCP_DEBUG(()) << getpid() <<" IS THE rpc RPC PID " <<
            //        std::this_thread::get_id()
            //        <<" is the thread ID";
            //             m_qk->set(other_time+delay - sc_core::sc_time_stamp());
            //             SCP_DEBUG(()) << "WHAT Sync";
            //             m_qk->sync();
            //             m_qk->reset();
            initiator_sockets[id]->b_transport(trans, delay);
            //             SCP_DEBUG(()) << "WHAT";
            //             m_qk->sync();
        });
        t.from_tlm(trans);
        t.m_quantum_time = delay.to_seconds();
        //        SCP_DEBUG(()) << name() << " from_tlm " << txn_str(trans) ;
        return t;
    }

    /* Debug transport interface */
    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        SCP_DEBUG(()) << name() << " ->remote debug tlm " << txn_str(trans);
        tlm_generic_payload_rpc t;

        t.from_tlm(trans);
        tlm_generic_payload_rpc r = client->call("dbg_tspt", id, t)
                                        .template as<tlm_generic_payload_rpc>();

        r.update_to_tlm(trans);
        SCP_DEBUG(()) << name() << " <-remote debug tlm done " << txn_str(trans);
        // this is not entirely accurate, but see below
        return trans.get_response_status() == tlm::TLM_OK_RESPONSE ? trans.get_data_length() : 0;
    }
    tlm_generic_payload_rpc transport_dbg_rpc(int id, tlm_generic_payload_rpc t)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);
        int ret_len;
        SCP_DEBUG(()) << name() << " remote-> debug tlm " << txn_str(trans);
        //        m_sc.run_on_sysc([&] {
        ret_len = initiator_sockets[id]->transport_dbg(trans);
        //            });
        t.from_tlm(trans);
        SCP_DEBUG(()) << name() << " remote<- debug tlm done " << txn_str(trans);

        if (!(trans.get_data_length() == ret_len ||
              trans.get_response_status() != tlm::TLM_OK_RESPONSE)) {
            assert(false);
            SCP_WARN(()) << "debug transaction not able to access required length of data.";
        }
        return t;
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        tlm::tlm_dmi* c;
        SCP_DEBUG(()) << " " << name() << " get_direct_mem_ptr to address "
                     << "0x" << std::hex << trans.get_address();

#ifdef DMICACHE
        c = in_cache(trans.get_address());
        if (c) {
            //            SCP_DEBUG(()) << "In Cache " << std::hex << c->get_start_address() << "
            //            - " << std::hex << c->get_end_address() ;
            dmi_data = *c;
            return !(dmi_data.is_none_allowed());
        }
#endif
        tlm_generic_payload_rpc t;
        t.from_tlm(trans);
        //        SCP_DEBUG(()) << name() << " DMI socket ID " << id << " From_tlm " <<
        //        txn_str(trans)
        //        ;
        tlm_dmi_rpc r = client->call("dmi_req", id, t).template as<tlm_dmi_rpc>();
        if (r.m_shmem_size == 0) {
            SCP_DEBUG(()) << name() << "DMI OK, but no shared memory available?"
                          << trans.get_address();
            return false;
        }
        //        SCP_DEBUG(()) << "Got " << std::hex << r.m_dmi_start_address << " - " <<
        //        std::hex << r.m_dmi_end_address ; c = in_cache(r.m_shmem_fn); if (c) {
        //            dmi_data = *c;
        //        } else {
        r.to_tlm(dmi_data);
//            SCP_DEBUG(()) << "Adding " << r.m_shmem_fn << " " << dmi_data.get_start_address()
//                      << " to cache";
#ifdef DMICACHE
        assert(m_dmi_cache.count(dmi_data.get_start_address()) == 0);
        m_dmi_cache[dmi_data.get_start_address()] = dmi_data;
#endif
        //        }
        //        SCP_DEBUG(()) << name() << "DMI to " <<trans.get_address()<<" status "
        //        <<!(dmi_data.is_none_allowed()) <<" range " << std::hex <<
        //        dmi_data.get_start_address() << " - " << std::hex << dmi_data.get_end_address() <<
        //        "";
        return !(dmi_data.is_none_allowed());
    }

    tlm_dmi_rpc get_direct_mem_ptr_rpc(int id, tlm_generic_payload_rpc t)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);

        SCP_DEBUG(()) << " " << name() << " get_direct_mem_ptr " << txn_str(trans);

        tlm::tlm_dmi dmi_data;
        tlm_dmi_rpc ret;
        ret.m_shmem_size = 0;
        if (initiator_sockets[id]->get_direct_mem_ptr(trans, dmi_data)) {
            ShmemIDExtension* ext = trans.get_extension<ShmemIDExtension>();
            if (!ext)
                return ret;
            ret.from_tlm(dmi_data, ext);
        }
        return ret;
    }

    /* Invalidate DMI Interface */
    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        SCP_DEBUG(()) << " " << name() << " invalidate_direct_mem_ptr "
                     << " start address 0x" << std::hex << start << " end address 0x" << std::hex
                     << end;
        client->call("dmi_inv", start, end);
    }
    void invalidate_direct_mem_ptr_rpc(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        SCP_DEBUG(()) << " " << name() << " invalidate_direct_mem_ptr "
                     << " start address 0x" << std::hex << start << " end address 0x" << std::hex
                     << end;
#ifdef DMICACHE
        cache_clean(start, end);
#endif
        for (int i = 0; i < target_sockets.size(); i++) {
            target_sockets[i]->invalidate_direct_mem_ptr(start, end);
        }
    }

    str_pairs get_cci_db()
    {
        std::string n = std::string(name());
        std::string parent = std::string(n).substr(0, n.find_last_of("."));

        str_pairs ret;
        for (auto p : m_broker.get_unconsumed_preset_values()) {
            std::string name = p.first;
            std::string k = p.first;
            if (k.find(parent) == 0) {
                k = k.substr(parent.length() + 1);
                ret.push_back(std::make_pair("$" + k, p.second.to_json()));
                // mark parameters in the remote as 'used'
                m_broker.ignore_unconsumed_preset_values(
                    [name](const std::pair<std::string, cci::cci_value>& iv) -> bool {
                        return iv.first == name;
                    });
            } else if (k.find('.') == std::string::npos) {
                ret.push_back(std::make_pair(k, p.second.to_json()));
            }
        }
        return ret;
    }

    void set_cci_db(str_pairs db)
    {
        std::string modname = std::string(name());
        std::string parent = std::string(modname).substr(0, modname.find_last_of("."));

        for (auto p : db) {
            std::string parname;
            if (p.first[0] == '$') {
                parname = parent + "." + p.first.substr(1);
            } else if (p.first[0] == '@') {
                parname = modname + "." + p.first.substr(1);
            } else {
                parname = p.first;
            }
            m_broker.set_preset_cci_value(parname, cci::cci_value::from_json(p.second));

            SCP_DEBUG(()) << "Setting " << parname << " to " << p.second;
        }
    }

public:
    PassRPC(const sc_core::sc_module_name& nm, std::string exec_path = "",
            const std::vector<std::string>& extra_args = std::vector<std::string>())
        : sc_core::sc_module(nm)
        , m_broker(cci::cci_get_broker())
        , initiator_sockets("initiator_socket", TLMPORTS,
                            [this](const char* n, int i) {
                                return new initiator_socket_spying(
                                    n, [&](std::string s) -> void { remote_register_boundto(s); });
                            })
        , target_sockets("target_socket", TLMPORTS,
                         [this](const char* n, int i) { return new tlm_target_socket(n); })
        , initiator_signal_sockets(
              "initiator_signal_socket", SIGNALS,
              [this](const char* n, int i) { return new InitiatorSignalSocket<bool>(n); })
        , target_signal_sockets(
              "target_signal_socket", SIGNALS,
              [this](const char* n, int i) { return new TargetSignalSocket<bool>(n); })
        , p_cport("client_port", 0,
                  "The port that should be used to connect this client to the "
                  "remote server")
        , p_sport("server_port", 0, "The port that should be used to server on")
        , p_exec_path("exec_path", exec_path,
                      "The path to the executable that should be started by "
                      "the bridge")
        , p_sync_policy("sync_policy", "multithread-unconstrained", "Sync policy for the remote")
    {
        SigHandler::get().add_sig_handler(SIGINT, SigHandler::Handler_CB::PASS);
        SCP_DEBUG(()) << "PassRPC constructor";
        SCP_DEBUG(()) << getpid() << " IS THE RPC PID " << std::this_thread::get_id()
                      << " is the thread ID";

        // always serve on a new port.
        server = new rpc::server(p_sport);
        server->suppress_exceptions(true);
        p_sport = server->port();
        assert(p_sport > 0);

        if (p_cport.get_value() == 0 && getenv((std::string(GS_Process_Server_Port) + std::to_string(getpid())).c_str())) {
            p_cport = std::stoi(std::string(getenv((std::string(GS_Process_Server_Port) + std::to_string(getpid())).c_str())));
        }

        // other end contacted us, connect to their port
        // and return back the cci database
        server->bind("reg", [&](int port) {
            SCP_DEBUG(()) << "reg " << name() << " 1";
            assert(p_cport == 0 && client == nullptr);
            p_cport = port;
            if (!client)
                client = new rpc::client("localhost", p_cport);
            std::unique_lock<std::mutex> ul(client_conncted_mut);
            is_client_connected.notify_one();
            ul.unlock();
            // we are not interested in the return future from async_call
            client->async_call("sock_pair", pahandler.get_sockpair_fd0(),
                               pahandler.get_sockpair_fd1());
            return get_cci_db();
        });

        // would it be better to have a 'remote cci broker' that connected back,

        server->bind("cci_db", [&](str_pairs db) {
            set_cci_db(db);
            return;
        });

        server->bind("status", [&](int s) {
            SCP_DEBUG(()) << "SIMULATION STATE " << name() << " to status " << s;
            assert(s > m_remote_status);
            m_remote_status = static_cast<sc_core::sc_status>(s);
            std::lock_guard<std::mutex> lg(sc_status_mut);
            is_sc_status_set.notify_one();
            return;
        });

        server->bind("b_tspt", [&](int id, tlm_generic_payload_rpc txn) {
            try {
                return PassRPC::b_transport_rpc(id, txn);
            } catch (std::runtime_error const& e) {
                std::cerr << "Main Error: '" << e.what() << "'\n";
                exit(1);
            } catch (const std::exception& exc) {
                std::cerr << "main Error: '" << exc.what() << "'\n";
                exit(1);
            } catch (...) {
                std::cerr << "Unknown error (main.cc)!\n";
                exit(1);
            }
        });

        server->bind("dbg_tspt", [&](int id, tlm_generic_payload_rpc txn) {
            SCP_DEBUG(()) << "Got DBG Tspt";
            return PassRPC::transport_dbg_rpc(id, txn);
        });

        server->bind("dmi_inv", [&](uint64_t start, uint64_t end) {
            return PassRPC::invalidate_direct_mem_ptr_rpc(start, end);
        });

        server->bind("dmi_req", [&](int id, tlm_generic_payload_rpc txn) {
            return PassRPC::get_direct_mem_ptr_rpc(id, txn);
        });

        server->bind("exit", [&](int i) {
            SCP_DEBUG(()) << "exit " << name();
            m_sc.run_on_sysc([&] {
                rpc::this_session().post_exit();
                delete client;
                client = nullptr;
                sc_core::sc_stop();
            });
            m_qk->stop();
            return;
        });

        server->bind("signal", [&](int i, bool v) {
            m_sc.run_on_sysc([&] { initiator_signal_sockets[i]->write(v); });
            return;
        });

        server->bind("sock_pair", [&](int sock_fd0, int sock_fd1) {
            pahandler.recv_sockpair_fds_from_remote(sock_fd0, sock_fd1);
            pahandler.check_parent_conn_nth([&]() {
                std::cerr << "remote process (" << getpid() << ") detected parent ("
                          << pahandler.get_ppid() << ") exit!" << std::endl;
            });
            return;
        });

        for (int i = 0; i < TLMPORTS; i++) {
            target_sockets[i].register_b_transport(this, &PassRPC::b_transport, i);
            target_sockets[i].register_transport_dbg(this, &PassRPC::transport_dbg, i);
            target_sockets[i].register_get_direct_mem_ptr(this, &PassRPC::get_direct_mem_ptr, i);

            initiator_sockets[i].register_invalidate_direct_mem_ptr(
                this, &PassRPC::invalidate_direct_mem_ptr);
        }

        for (int i = 0; i < SIGNALS; i++) {
            target_signal_sockets[i].register_value_changed_cb(
                [&, i](bool value) { client->send("signal", i, value); });
        }

        m_qk = tlm_quantumkeeper_factory(p_sync_policy);
        m_qk->reset();
        server->async_run(1);

        if (p_cport) {
            SCP_INFO(()) << "Connecting client on port " << p_cport;
            if (!client)
                client = new rpc::client("localhost", p_cport);
            set_cci_db(client->call("reg", (int)p_sport).as<str_pairs>());
        }

        if (!exec_path.empty()) {
            SCP_INFO(()) << "Forking remote " << exec_path;
            pahandler.init_peer_conn_checker();

            std::vector<const char*> argp;
            argp.reserve(extra_args.size() + 2);
            argp.push_back(exec_path.c_str());
            std::transform(extra_args.begin(), extra_args.end(),std::back_inserter(argp), [](const std::string &s){return s.c_str();});
            argp.push_back(0);

            m_child_pid = fork();
            if (m_child_pid > 0) {
                pahandler.setup_parent_conn_checker();
                SigHandler::get().set_nosig_chld_stop();
                SigHandler::get().add_sig_handler(SIGCHLD, SigHandler::Handler_CB::EXIT);
            } else if (m_child_pid == 0) {
                setenv((std::string(GS_Process_Server_Port) + std::to_string(getpid())).c_str(), std::to_string(p_sport).c_str(), 1);
                execv(exec_path.c_str(), const_cast<char**>(&argp[0]));

                // execlp("lldb", "lldb", "--", exec_path.c_str(), exec_path.c_str(), "-p",
                // conf_arg, nullptr);
                SCP_FATAL(()) << "Unable to exec the remote child process, error: " << std::strerror(errno);
            } else {
                SCP_FATAL(()) << "failed to fork remote process, error: " << std::strerror(errno);
            }
        }

        // Make sure by now the client is connected so we can send/recieve.
        std::unique_lock<std::mutex> ul(client_conncted_mut);
        is_client_connected.wait(ul, [&]() { return p_cport > 0; });
        ul.unlock();
        send_status();
    }
    PassRPC(const sc_core::sc_module_name& nm, int port)
        : PassRPC(nm, "", port){}; // convenience constructor

    void send_status()
    {
        SCP_DEBUG(()) << "SIMULATION STATE send " << name() << " to status "
                      << sc_core::sc_get_status();
        client->call("status", static_cast<int>(sc_core::sc_get_status()));
        std::unique_lock<std::mutex> ul(sc_status_mut);
        is_sc_status_set.wait(ul, [&]() { return m_remote_status >= sc_core::sc_get_status(); });
        ul.unlock();
        SCP_DEBUG(()) << "SIMULATION STATE synced " << name() << " to status "
                      << sc_core::sc_get_status();
    }
    void before_end_of_elaboration() { send_status(); }
    void end_of_elaboration() { send_status(); }
    void start_of_simulation()
    {
        send_status();
        m_qk->start();
    }

    PassRPC() = delete;
    PassRPC(const PassRPC&) = delete;
    ~PassRPC()
    {
        m_qk->stop();
        SCP_DEBUG(()) << "EXIT " << name();
        if (client) {
            client->async_call("exit", 0);
            delete client;
            client = nullptr;
        }
        if (server)
            delete server;
#ifdef DMICACHE
        m_dmi_cache.clear();
#endif
    }

    void end_of_simulation()
    {
        m_qk->stop();
        if (client) {
            client->async_call("exit", 0);
            delete client;
            client = nullptr;
            sleep(1);
        }
    }
};

} // namespace gs
#endif
