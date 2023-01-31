/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

#ifndef _GREENSOCS_BASE_COMPONENTS_DMI_CONVERTER_H
#define _GREENSOCS_BASE_COMPONENTS_DMI_CONVERTER_H

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <map>
#include <string>

namespace gs {

template <unsigned int TLMPORTS = 1, unsigned int BUSWIDTH = 32>
class DMIConverter : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    using MOD = DMIConverter<TLMPORTS, BUSWIDTH>;
    using tlm_initiator_socket_t = tlm_utils::simple_initiator_socket_b<
        MOD, BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_t = tlm_utils::simple_target_socket_tagged_b<
        MOD, BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;
    // These sockets are kept public for binding.
    sc_core::sc_vector<tlm_initiator_socket_t> initiator_sockets;
    sc_core::sc_vector<tlm_target_socket_t> target_sockets;

public:
    DMIConverter(const sc_core::sc_module_name& nm)
        : sc_core::sc_module(nm)
        , initiator_sockets("initiator_socket", TLMPORTS,
                            [this](const char* n, int i) { return new tlm_initiator_socket_t(n); })
        , target_sockets("target_socket", TLMPORTS,
                         [this](const char* n, int i) { return new tlm_target_socket_t(n); })
    {
        SCP_DEBUG(()) << "DMIConverter constructor";
        for (int i = 0; i < TLMPORTS; i++) {
            target_sockets[i].register_b_transport(this, &DMIConverter::b_transport, i);
            target_sockets[i].register_transport_dbg(this, &DMIConverter::transport_dbg, i);
            target_sockets[i].register_get_direct_mem_ptr(this, &DMIConverter::get_direct_mem_ptr,
                                                          i);
            initiator_sockets[i].register_invalidate_direct_mem_ptr(
                this, &DMIConverter::invalidate_direct_mem_ptr);
        }
    }

    ~DMIConverter() {}

private:
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        uint64_t addr = trans.get_address();
        uint64_t len = trans.get_data_length();
        tlm::tlm_command cmd = trans.get_command();
        unsigned char* trans_data_ptr = trans.get_data_ptr();
        uint64_t remaining_len = len;
        uint64_t current_block_len = 0;
        tlm::tlm_dmi* dmi_data;
        bool is_cache_used = false;
        while (remaining_len) {
            dmi_data = in_cache(addr);
            if (dmi_data && is_dmi_access_type_granted(*dmi_data, cmd)) {
                sc_dt::uint64 start_addr = dmi_data->get_start_address();
                sc_dt::uint64 end_addr = dmi_data->get_end_address();
                unsigned char* dmi_ptr = dmi_data->get_dmi_ptr();
                current_block_len = (end_addr - addr) + 1;
                uint64_t iter_len = (remaining_len > current_block_len ? current_block_len
                                                                       : remaining_len);
                switch (cmd) {
                case tlm::TLM_IGNORE_COMMAND:
                    return;
                case tlm::TLM_WRITE_COMMAND:
                    SCP_INFO(()) << "(write request) cache is used to write " << std::hex
                                 << iter_len << " bytes starting from: 0x" << std::hex << addr
                                 << ", cache block used starts at: 0x" << std::hex << start_addr
                                 << " and ends at: 0x" << std::hex << end_addr;
                    memcpy(reinterpret_cast<unsigned char*>(&dmi_ptr[addr - start_addr]),
                           reinterpret_cast<unsigned char*>(trans_data_ptr), iter_len);
                    break;
                case tlm::TLM_READ_COMMAND:
                    SCP_INFO(()) << "(read request) cache is used to read " << std::hex << iter_len
                                 << " bytes starting from: 0x" << std::hex << addr
                                 << ", cache block used starts at: 0x" << std::hex << start_addr
                                 << " and ends at: 0x" << std::hex << end_addr;
                    memcpy(reinterpret_cast<unsigned char*>(trans_data_ptr),
                           reinterpret_cast<unsigned char*>(&dmi_ptr[addr - start_addr]), iter_len);
                    break;
                default:
                    SCP_FATAL(()) << "invalid tlm_command at address: 0x" << std::hex << addr;
                    break;
                }
                remaining_len = (iter_len == remaining_len ? 0 : remaining_len - iter_len);
                addr += iter_len;
                trans_data_ptr += iter_len;
                len = remaining_len;
                if (remaining_len == 0) {
                    trans.set_dmi_allowed(true);
                    trans.set_response_status(tlm::TLM_OK_RESPONSE);
                }
                is_cache_used = true;
                SCP_INFO(()) << "remaining_len: " << remaining_len;
            } else {
                tlm::tlm_dmi t_dmi_data;
                tlm::tlm_generic_payload t_trans;
                t_dmi_data.init();
                if (is_cache_used) {
                    t_trans.deep_copy_from(trans);
                    t_trans.set_address(addr);
                    t_trans.set_data_length(len);
                    t_trans.set_data_ptr(trans_data_ptr);
                }
                bool dmi_ptr_valid = initiator_sockets[id]->get_direct_mem_ptr(
                    (is_cache_used ? t_trans : trans), t_dmi_data);
                if (dmi_ptr_valid && is_dmi_access_type_granted(t_dmi_data, cmd)) {
                    SCP_INFO(())
                        << get_access_type_str(cmd)
                        << " data is not in cache, DMI request is successful, granted start addr: "
                           "0x"
                        << std::hex << t_dmi_data.get_start_address() << " granted end addr: 0x"
                        << std::hex << t_dmi_data.get_end_address();
                    m_dmi_cache[t_dmi_data.get_start_address()] = t_dmi_data;
                } else {
                    initiator_sockets[id]->b_transport((is_cache_used ? t_trans : trans), delay);
                    SCP_INFO(())
                        << get_access_type_str(cmd)
                        << " data is not in cache, DMI request failed, b_transport used, len: "
                        << std::hex
                        << (is_cache_used ? t_trans.get_data_length() : trans.get_data_length())
                        << " addr: 0x" << std::hex
                        << (is_cache_used ? t_trans.get_address() : trans.get_address());
                    trans.set_dmi_allowed(false);
                    if (is_cache_used)
                        trans.set_response_status(t_trans.get_response_status());
                    break;
                }
            }
        }
    }

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        SCP_DEBUG(()) << "calling dbg_transport: ";
        return initiator_sockets[id]->transport_dbg(trans);
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        SCP_DEBUG(()) << "DMI to " << trans.get_address() << " range " << std::hex
                      << dmi_data.get_start_address() << " - " << std::hex
                      << dmi_data.get_end_address();
        auto c_dmi_data = in_cache(trans.get_address());
        if (c_dmi_data) {
            dmi_data = *c_dmi_data;
            return !(dmi_data.is_none_allowed());
        } else {
            auto dmi_ptr_valid = initiator_sockets[id]->get_direct_mem_ptr(trans, dmi_data);
            if (dmi_ptr_valid)
                m_dmi_cache[dmi_data.get_start_address()] = dmi_data;
            return dmi_ptr_valid;
        }
    }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        SCP_DEBUG(()) << " invalidate_direct_mem_ptr "
                      << " start address 0x" << std::hex << start << " end address 0x" << std::hex
                      << end;

        cache_clean(start, end);
        for (int i = 0; i < target_sockets.size(); i++) {
            target_sockets[i]->invalidate_direct_mem_ptr(start, end);
        }
    }

    bool is_dmi_access_type_granted(const tlm::tlm_dmi& dmi_data, const tlm::tlm_command& cmd)
    {
        switch (cmd) {
        case tlm::TLM_WRITE_COMMAND:
            return dmi_data.is_write_allowed();
        case tlm::TLM_READ_COMMAND:
            return dmi_data.is_read_allowed();
        default:
            return false;
        }
    }

    std::string get_access_type_str(const tlm::tlm_command& cmd)
    {
        std::string ret_str;
        switch (cmd) {
        case tlm::TLM_WRITE_COMMAND:
            ret_str = "(write request)";
            break;
        case tlm::TLM_READ_COMMAND:
            ret_str = "(read request)";
            break;
        case tlm::TLM_IGNORE_COMMAND:
            ret_str = "(ignore request)";
            break;
        default:
            ret_str = "(invalid tlm_command)";
            break;
        }
        return ret_str;
    }

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

private:
    std::map<uint64_t, tlm::tlm_dmi> m_dmi_cache;
};
} // namespace gs

#endif
