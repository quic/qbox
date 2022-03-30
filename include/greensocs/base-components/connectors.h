/*
* Copyright (c) 2022 GreenSocs
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
* You may obtain a copy of the Apache License at
* http://www.apache.org/licenses/LICENSE-2.0
*/

#ifndef _GREENSOCS_BASE_COMPONENTS_CONNECTORS_H
#define _GREENSOCS_BASE_COMPONENTS_CONNECTORS_H

#include <cci_configuration>
#include <systemc>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

namespace gs {
template <unsigned int BUSWIDTH = 32>
class pass : public sc_core::sc_module {

    /* Alias from - to */
    void alias_preset_param(std::string a, std::string b, bool required = false)
    {

        if (m_broker.has_preset_value(a)) {
            m_broker.set_preset_cci_value(b, m_broker.get_preset_cci_value(a));
            m_broker.lock_preset_value(a);
            m_broker.ignore_unconsumed_preset_values(
                [a](const std::pair<std::string, cci::cci_value>& iv) -> bool { return iv.first == a; });
        } else {
            if (required) {
                SC_REPORT_FATAL("pass ", (std::string(name()) + " Can't find " + a).c_str());
            }
        }
    }

    template <typename MOD>
    class initiator_socket_spying
        : public tlm_utils::simple_initiator_socket<MOD, BUSWIDTH> {
        using typename tlm_utils::simple_initiator_socket<MOD, BUSWIDTH>::base_target_socket_type;

        const std::function<void(std::string)> register_cb;

    public:
        initiator_socket_spying(const char* name,
            const std::function<void(std::string)>& f)
            : tlm_utils::simple_initiator_socket<MOD, BUSWIDTH>::simple_initiator_socket(name)
            , register_cb(f)
        {
        }

        void bind(base_target_socket_type& socket)
        {
            tlm_utils::simple_initiator_socket<MOD, BUSWIDTH>::bind(socket);
            register_cb(socket.get_base_export().name());
        }
    };

    /* NB use the EXPORT name, so as not to be hassled by the _port_0*/
    std::string nameFromSocket(std::string s)
    {
        return s;
    }
    void register_boundto(std::string s)
    {
        s = nameFromSocket(s);
        alias_preset_param(s + ".address", std::string(name()) + ".target_socket.address");
        alias_preset_param(s + ".size", std::string(name()) + ".target_socket.size");
        alias_preset_param(s + ".relative_addresses", std::string(name()) + ".target_socket.relative_addresses");
    }

public:
    initiator_socket_spying<pass<BUSWIDTH>> initiator_socket;
    tlm_utils::simple_target_socket<pass<BUSWIDTH>, BUSWIDTH> target_socket;
    cci::cci_param<bool> p_verbose;

private:
    void b_transport(tlm::tlm_generic_payload& trans,
        sc_core::sc_time& delay)
    {
        if (p_verbose) {
            std::stringstream info;
            info << " " << name()
                 << " b_transport to address "
                 << "0x" << std::hex << trans.get_address();
            SC_REPORT_INFO("pass", info.str().c_str());
        }
        initiator_socket->b_transport(trans, delay);
    }

    unsigned int transport_dbg(tlm::tlm_generic_payload& trans)
    {
        if (p_verbose) {
            std::stringstream info;
            info << " " << name()
                 << " transport_dbg to address "
                 << "0x" << std::hex << trans.get_address();
            SC_REPORT_INFO("pass", info.str().c_str());
        }
        return initiator_socket->transport_dbg(trans);
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans,
        tlm::tlm_dmi& dmi_data)
    {
        if (p_verbose) {
            std::stringstream info;
            info << " " << name()
                 << " get_direct_mem_ptr to address "
                 << "0x" << std::hex << trans.get_address();
            SC_REPORT_INFO("pass", info.str().c_str());
        }
        return initiator_socket->get_direct_mem_ptr(trans, dmi_data);
    }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start,
        sc_dt::uint64 end)
    {
        if (p_verbose) {
            std::stringstream info;
            info << " " << name()
                 << " invalidate_direct_mem_ptr "
                 << " start address 0x" << std::hex << start
                 << " end address 0x" << std::hex << end;
            SC_REPORT_INFO("pass", info.str().c_str());
        }
        target_socket->invalidate_direct_mem_ptr(start, end);
    }

    cci::cci_broker_handle m_broker;

public:
    explicit pass(const sc_core::sc_module_name& nm, bool _verbose = false)
        : sc_core::sc_module(nm)
        , initiator_socket("initiator_socket", [&](std::string s) -> void { register_boundto(s); })
        , target_socket("target_socket")
        , p_verbose("verbose", _verbose, "print all transactions as they pass through")
        , m_broker(cci::cci_get_broker())
    {
        target_socket.register_b_transport(this, &pass::b_transport);
        target_socket.register_transport_dbg(this, &pass::transport_dbg);
        target_socket.register_get_direct_mem_ptr(this, &pass::get_direct_mem_ptr);
        initiator_socket.register_invalidate_direct_mem_ptr(this, &pass::invalidate_direct_mem_ptr);
    }
    pass() = delete;
    pass(const pass&) = delete;
    ~pass() { }
};

}
#endif
