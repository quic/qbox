/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _GREENSOCS_BASE_COMPONENTS_PASS_H
#define _GREENSOCS_BASE_COMPONENTS_PASS_H

#include <cci_configuration>
#include <systemc>
#include <scp/report.h>
#include <scp/helpers.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <module_factory_registery.h>
#include <tlm_sockets_buswidth.h>

#include <iomanip>

namespace gs {
template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class pass : public sc_core::sc_module
{
    SCP_LOGGER(());

    /* Alias from - to */
    void alias_preset_param(std::string a, std::string b, bool required = false)
    {
        if (gs::cci_get<std::string>(m_broker, a, b)) {
            m_broker.set_preset_cci_value(b, m_broker.get_preset_cci_value(a));
            m_broker.lock_preset_value(a);
            m_broker.ignore_unconsumed_preset_values(
                [a](const std::pair<std::string, cci::cci_value>& iv) -> bool { return iv.first == a; });
        } else {
            if (required) {
                SCP_FATAL(()) << " Can't find " << a;
            }
        }
    }

    template <typename MOD>
    class initiator_socket_spying : public tlm_utils::simple_initiator_socket<MOD, BUSWIDTH>
    {
        using typename tlm_utils::simple_initiator_socket<MOD, BUSWIDTH>::base_target_socket_type;

        const std::function<void(std::string)> register_cb;

    public:
        initiator_socket_spying(const char* name, const std::function<void(std::string)>& f)
            : tlm_utils::simple_initiator_socket<MOD, BUSWIDTH>::simple_initiator_socket(name), register_cb(f)
        {
        }

        void bind(base_target_socket_type& socket)
        {
            tlm_utils::simple_initiator_socket<MOD, BUSWIDTH>::bind(socket);
            register_cb(socket.get_base_export().name());
        }

        // hierarchial binding
        void bind(tlm::tlm_initiator_socket<BUSWIDTH>& socket)
        {
            tlm_utils::simple_initiator_socket<MOD, BUSWIDTH>::bind(socket);
            register_cb(socket.get_base_port().name());
        }
    };

    /* NB use the EXPORT name, so as not to be hassled by the _port_0*/
    std::string nameFromSocket(std::string s) { return s; }
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

private:
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        SCP_INFO(()) << "calling b_transport: " << scp::scp_txn_tostring(trans);
        initiator_socket->b_transport(trans, delay);
        SCP_INFO(()) << "returning from b_transport: " << scp::scp_txn_tostring(trans);
    }

    unsigned int transport_dbg(tlm::tlm_generic_payload& trans)
    {
        SCP_INFO(()) << "calling dbg_transport: " << scp::scp_txn_tostring(trans);
        return initiator_socket->transport_dbg(trans);
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        SCP_INFO(()) << "calling get_direct_mem_ptr: " << scp::scp_txn_tostring(trans);
        return initiator_socket->get_direct_mem_ptr(trans, dmi_data);
    }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        SCP_INFO(()) << std::hex << "calling invalidate_direct_mem_ptr: 0x" << start << " - 0x" << end;
        target_socket->invalidate_direct_mem_ptr(start, end);
    }

    cci::cci_broker_handle m_broker;

public:
    explicit pass(const sc_core::sc_module_name& nm)
        : sc_core::sc_module(nm)
        , initiator_socket("initiator_socket", [&](std::string s) -> void { register_boundto(s); })
        , target_socket("target_socket")
        , m_broker(cci::cci_get_broker())
    {
        SCP_DEBUG(()) << "pass constructor";

        target_socket.register_b_transport(this, &pass::b_transport);
        target_socket.register_transport_dbg(this, &pass::transport_dbg);
        target_socket.register_get_direct_mem_ptr(this, &pass::get_direct_mem_ptr);
        initiator_socket.register_invalidate_direct_mem_ptr(this, &pass::invalidate_direct_mem_ptr);
    }
    pass() = delete;
    pass(const pass&) = delete;
    ~pass() {}
};

} // namespace gs

extern "C" void module_register();

#endif
