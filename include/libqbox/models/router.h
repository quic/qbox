/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps
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

#pragma once

#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

template<unsigned int BUSWIDTH = 32>
struct Router : sc_core::sc_module
{
private:
    typedef tlm::tlm_base_target_socket_b<BUSWIDTH,
            tlm::tlm_fw_transport_if<>,
            tlm::tlm_bw_transport_if<> > target;

    std::vector<tlm_utils::simple_initiator_socket_tagged<Router> *> initiator_socket;

    struct target_info {
        size_t index;
        target *t;
        sc_dt::uint64 address;
        sc_dt::uint64 size;
    };
    std::vector<target_info> targets;

public:
    tlm_utils::simple_target_socket<Router> target_socket;

    SC_CTOR(Router)
        : target_socket("target_socket")
    {
        target_socket.register_b_transport(this, &Router::b_transport);
        target_socket.register_get_direct_mem_ptr(this, &Router::get_direct_mem_ptr);
    }

    void before_end_of_elaboration()
    {
        for (size_t i = 0; i < targets.size(); i++) {
            tlm_utils::simple_initiator_socket_tagged<Router> *socket =
                new tlm_utils::simple_initiator_socket_tagged<Router>(sc_core::sc_gen_unique_name("socket"));
            socket->bind(*targets.at(i).t);
            initiator_socket.push_back(socket);
        }
    }

    virtual void add_target(target &t, uint64_t address, uint64_t size)
    {
		struct target_info ti;
		ti.index = targets.size();
		ti.t = &t;
		ti.address = address;
		ti.size = size;
        targets.push_back(ti);
    }

    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        sc_dt::uint64 addr = trans.get_address();
        sc_dt::uint64 target_addr;
        unsigned int target_nr;

        bool success = decode_address(addr, target_addr, target_nr);
        if (!success) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        trans.set_address(target_addr);

        (*initiator_socket[target_nr])->b_transport(trans, delay);
    }

    virtual bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans,
            tlm::tlm_dmi& dmi_data)
    {
        sc_dt::uint64 target_addr;
        unsigned int target_nr;

        bool success = decode_address(trans.get_address(), target_addr, target_nr);
        if (!success) {
            return false;
        }

        trans.set_address(target_addr);

        bool status = (*initiator_socket[target_nr])->get_direct_mem_ptr(trans, dmi_data);
        dmi_data.set_start_address(compose_address(target_nr, dmi_data.get_start_address()));
        dmi_data.set_end_address(compose_address(target_nr, dmi_data.get_end_address()));
        return status;
    }

    bool decode_address(sc_dt::uint64 addr, sc_dt::uint64& addr_out, unsigned int &index_out)
    {
        for (unsigned int i = 0; i < targets.size(); i++) {
            struct target_info &ti = targets.at(i);
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
        return targets[target_nr].address + address;
    }
};
