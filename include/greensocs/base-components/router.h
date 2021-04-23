/*
 *  This file is part of GreenSocs base-components
 *  Copyright (c) 2020-2021 GreenSocs
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_ROUTER_H
#define _GREENSOCS_BASE_COMPONENTS_ROUTER_H

#include <cinttypes>
#include <vector>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

#include <greensocs/libgsutils.h>

template <unsigned int BUSWIDTH = 32>
class Router : public sc_core::sc_module {
private:
    using TargetSocket =
        tlm::tlm_base_target_socket_b<BUSWIDTH,
                                      tlm::tlm_fw_transport_if<>,
                                      tlm::tlm_bw_transport_if<> >;

    using InitiatorSocket =
        tlm::tlm_base_initiator_socket_b<BUSWIDTH,
                                         tlm::tlm_fw_transport_if<>,
                                         tlm::tlm_bw_transport_if<> >;

    struct TargetInfo {
        size_t index;
        TargetSocket *t;
        sc_dt::uint64 address;
        sc_dt::uint64 size;
    };

    struct InitiatorInfo {
        size_t index;
        InitiatorSocket *i;
    };

    std::vector<tlm_utils::simple_target_socket_tagged<Router> *> m_target_sockets;
    std::vector<tlm_utils::simple_initiator_socket_tagged<Router> *> m_initiator_sockets;
    std::vector<TargetInfo> m_targets;
    std::vector<InitiatorInfo> m_initiators;

    void b_transport(int id, tlm::tlm_generic_payload &trans, sc_core::sc_time &delay)
    {
        sc_dt::uint64 addr = trans.get_address();
        sc_dt::uint64 target_addr;
        unsigned int target_nr;

        bool success = decode_address(addr, target_addr, target_nr);
        if (!success) {
            const char *cmd = "unknown";
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

            GS_LOG("Warning: '%s' access to unmapped address 0x%" PRIx64 " in '%s' module\n",
                   cmd, static_cast<uint64_t>(trans.get_address()), name());
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        trans.set_address(target_addr);

        (*m_initiator_sockets[target_nr])->b_transport(trans, delay);
    }

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload &trans)
    {
        sc_dt::uint64 addr = trans.get_address();
        sc_dt::uint64 target_addr;
        unsigned int target_nr;

        bool success = decode_address(addr, target_addr, target_nr);

        if (!success) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return 0;
        }

        trans.set_address(target_addr);

        return (*m_initiator_sockets[target_nr])->transport_dbg(trans);
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload &trans, tlm::tlm_dmi &dmi_data)
    {
        sc_dt::uint64 target_addr;
        unsigned int target_nr;

        bool success = decode_address(trans.get_address(), target_addr, target_nr);
        if (!success) {
            return false;
        }

        trans.set_address(target_addr);

        bool status = (*m_initiator_sockets[target_nr])->get_direct_mem_ptr(trans, dmi_data);
        dmi_data.set_start_address(compose_address(target_nr, dmi_data.get_start_address()));
        dmi_data.set_end_address(compose_address(target_nr, dmi_data.get_end_address()));
        return status;
    }

    void invalidate_direct_mem_ptr(int id, sc_dt::uint64 start, sc_dt::uint64 end)
    {
        sc_dt::uint64 bw_start_range = compose_address(id, start);
        sc_dt::uint64 bw_end_range = compose_address(id, end);

        for (auto *socket : m_target_sockets) {
            (*socket)->invalidate_direct_mem_ptr(bw_start_range, bw_end_range);
        }
    }

    bool decode_address(sc_dt::uint64 addr, sc_dt::uint64 &addr_out, unsigned int &index_out)
    {
        for (unsigned int i = 0; i < m_targets.size(); i++) {
            struct TargetInfo &ti = m_targets.at(i);
            if (addr >= ti.address && addr < (ti.address + ti.size)) {
                addr_out = addr - ti.address;
                index_out = i;
                return true;
            }
        }
        return false;
    }

    inline sc_dt::uint64 compose_address(unsigned int target_nr, sc_dt::uint64 address)
    {
        return m_targets[target_nr].address + address;
    }

protected:
    virtual void before_end_of_elaboration()
    {
        for (size_t i = 0; i < m_initiators.size(); i++) {
            tlm_utils::simple_target_socket_tagged<Router> *socket =
                    new tlm_utils::simple_target_socket_tagged<Router>(sc_core::sc_gen_unique_name("target"));
            socket->register_b_transport(this, &Router::b_transport, i);
            socket->register_transport_dbg(this, &Router::transport_dbg, i);
            socket->register_get_direct_mem_ptr(this, &Router::get_direct_mem_ptr, i);
            socket->bind(*m_initiators.at(i).i);
            m_target_sockets.push_back(socket);
        }

        for (size_t i = 0; i < m_targets.size(); i++) {
            tlm_utils::simple_initiator_socket_tagged<Router> *socket =
                    new tlm_utils::simple_initiator_socket_tagged<Router>(sc_core::sc_gen_unique_name("initiator"));
            socket->register_invalidate_direct_mem_ptr(this, &Router::invalidate_direct_mem_ptr, i);
            socket->bind(*m_targets.at(i).t);
            m_initiator_sockets.push_back(socket);
        }
    }

public:
    explicit Router(const sc_core::sc_module_name &nm)
            : sc_core::sc_module(nm)
    {}

    Router() = delete;

    Router(const Router &) = delete;

    virtual ~Router()
    {
        for (const auto &i: m_initiator_sockets) {
            delete i;
        }
        for (const auto &t: m_target_sockets) {
            delete t;
        }
    }

    void add_target(TargetSocket &t, uint64_t address, uint64_t size)
    {
        struct TargetInfo ti;
        ti.index = m_targets.size();
        ti.t = &t;
        ti.address = address;
        ti.size = size;
        m_targets.push_back(ti);
    }

    void add_initiator(InitiatorSocket &i)
    {
        struct InitiatorInfo ii;
        ii.index = m_initiators.size();
        ii.i = &i;
        m_initiators.push_back(ii);
    }
};

#endif
