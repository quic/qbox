/*
 * Copyright (c) 2024 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_REG_ROUTER_H
#define _GREENSOCS_BASE_COMPONENTS_REG_ROUTER_H

#include <cinttypes>
#include <iomanip>
#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>
#include <scp/helpers.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <cciutils.h>
#include <module_factory_registery.h>
#include <tlm_sockets_buswidth.h>
#include <gs_memory.h>
#include <router_if.h>
#include <vector>
#include <memory>
#include <map>

namespace gs {

template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class reg_router : public sc_core::sc_module, public gs::router_if<BUSWIDTH>
{
    SCP_LOGGER(());
    SCP_LOGGER((DMI), "dmi");

    using typename gs::router_if<BUSWIDTH>::target_info;
    using initiator_socket_type = typename gs::router_if<BUSWIDTH>::template multi_passthrough_initiator_socket_spying<
        reg_router<BUSWIDTH>>;
    using gs::router_if<BUSWIDTH>::bound_targets;

    void register_boundto(std::string s)
    {
        s = gs::router_if<BUSWIDTH>::nameFromSocket(s);
        target_info ti = { 0 };
        ti.name = s;
        ti.index = bound_targets.size();
        SCP_DEBUG(()) << "Connecting : " << ti.name;
        auto tmp = name();
        int i;
        for (i = 0; i < s.length(); i++)
            if (s[i] != tmp[i]) break;
        ti.shortname = s.substr(i);
        bound_targets.push_back(ti);
    }

public:
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        sc_dt::uint64 addr = trans.get_address();

        auto ti = decode_address(trans);

        if (!ti) {
            SCP_WARN(())("Attempt to access unknown location in register memory at offset 0x{:x}", addr);
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        bool found_cb = do_callbacks(trans, delay);
        if (trans.get_response_status() >= tlm::TLM_INCOMPLETE_RESPONSE) {
            SCP_TRACE(()) << "call b_transport: " << txn_to_str(trans, false, found_cb);
            if (ti->use_offset) trans.set_address(addr - ti->address);
            initiator_socket[ti->index]->b_transport(trans, delay);
            if (ti->use_offset) trans.set_address(addr);
            SCP_TRACE(()) << "b_transport returned: " << txn_to_str(trans, false, found_cb);
        }
        if (trans.get_response_status() >= tlm::TLM_OK_RESPONSE) {
            do_callbacks(trans, delay);
        }
        if (cb_targets.size() > 0) {
            trans.set_dmi_allowed(false);
        }
    }

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        sc_dt::uint64 addr = trans.get_address();
        // transport_dbg transactions should only be handled by register memory
        auto ti = decode_address(trans);

        if (!ti) {
            SCP_WARN(())("transport_dbg: Attempt to access unknown location in register memory at offset 0x{:x}", addr);
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return 0;
        }
        if (ti->use_offset) trans.set_address(addr - ti->address);
        SCP_TRACE(()) << "[REG-MEM] call transport_dbg [txn]: " << scp::scp_txn_tostring(trans);
        unsigned int ret = initiator_socket[ti->index]->transport_dbg(trans);
        if (ti->use_offset) trans.set_address(addr);
        return ret;
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        sc_dt::uint64 addr = trans.get_address();
        // DMI transactions should only be handled by register memory
        auto ti = decode_address(trans);
        if (!ti) {
            SCP_WARN(())
            ("get_direct_mem_ptr: Attempt to access unknown location in register memory at offset 0x{:x}", addr);
            return false;
        }
        if (ti->use_offset) trans.set_address(addr - ti->address);
        SCP_TRACE(()) << "[REG-MEM] call get_direct_mem_ptr [txn]: " << scp::scp_txn_tostring(trans);
        bool ret = initiator_socket[ti->index]->get_direct_mem_ptr(trans, dmi_data);
        if (ti->use_offset) trans.set_address(addr);
        return ret;
    }

    std::string txn_to_str(tlm::tlm_generic_payload& trans, bool is_callback = false, bool found_callback = false)
    {
        std::stringstream ss;
        std::string cmd;
        switch (trans.get_command()) {
        case tlm::TLM_READ_COMMAND:
            cmd = "READ";
            break;
        case tlm::TLM_WRITE_COMMAND:
            cmd = "WRITE";
            break;
        case tlm::TLM_IGNORE_COMMAND:
            cmd = "IGNORE";
            break;
        default:
            cmd = "UNKNOWN";
            break;
        }
        if (is_callback && found_callback) {
            if (cmd != "IGNORE" && cmd != "UNKNOWN") {
                if ((trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE))
                    ss << "[PRE-" << cmd << "] Register callbacks [txn]: ";
                else if ((trans.get_response_status() == tlm::TLM_OK_RESPONSE))
                    ss << "[POST-" << cmd << "] Register callbacks [txn]: ";
            }
        } else if (found_callback) {
            ss << "[REG-MEM] (" << cmd << " TO REG-MEM BEFORE " << cmd << " CB) [txn]: ";
        } else {
            ss << "[REG-MEM] (NO CB FOUND) [txn]: ";
        }

        ss << scp::scp_txn_tostring(trans);
        return ss.str();
    }

    bool do_callbacks(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        if (cb_targets.empty()) {
            return false;
        }

        sc_dt::uint64 addr = trans.get_address();
        sc_dt::uint64 len = trans.get_data_length();
        target_info* ti;

        auto search = cb_targets.equal_range(addr);
        if (search.first != cb_targets.end() && search.first->first == addr) {
            if ((addr - search.first->first) < search.first->second->size) {
                ti = search.first->second;
            }
        } else {
            auto expected_node = std::prev(search.second);
            if ((addr - expected_node->first) < expected_node->second->size) {
                ti = expected_node->second;
            } else {
                return false;
            }
        }
        SCP_TRACE(()) << "call b_transport: " << txn_to_str(trans, true, true);
        if (ti->use_offset) trans.set_address(addr - ti->address);
        initiator_socket[ti->index]->b_transport(trans, delay);
        if (ti->use_offset) trans.set_address(addr);
        SCP_TRACE(()) << "b_transport returned : " << txn_to_str(trans, true, true);
        if (trans.get_response_status() <= tlm::TLM_GENERIC_ERROR_RESPONSE) {
            SCP_WARN(()) << "Accesing register callback at addr: " << std::hex << addr
                         << " returned with: " << trans.get_response_string();
        }
        return true;
    }

    target_info* decode_address(tlm::tlm_generic_payload& trans)
    {
        lazy_initialize();

        sc_dt::uint64 addr = trans.get_address();
        sc_dt::uint64 len = trans.get_data_length();

        for (auto ti : mem_targets) {
            if (addr >= ti->address && (addr - ti->address) < ti->size) {
                return ti;
            }
        }
        return nullptr;
    }

protected:
    virtual void before_end_of_elaboration()
    {
        if (!lazy_init) lazy_initialize();
    }

protected:
    void lazy_initialize()
    {
        if (initialized) return;
        initialized = true;

        for (auto& ti : bound_targets) {
            std::string name = ti.name;
            std::string src;

            ti.address = gs::cci_get<uint64_t>(m_broker, name + ".address");
            ti.size = gs::cci_get<uint64_t>(m_broker, name + ".size");
            ti.use_offset = gs::cci_get_d<bool>(m_broker, name + ".relative_addresses", true);
            ti.is_callback = gs::cci_get_d<bool>(m_broker, name + ".is_callback", false);

            SCP_INFO(()) << "Address map " << ti.name + " at"
                         << " address "
                         << "0x" << std::hex << ti.address << " size "
                         << "0x" << std::hex << ti.size << (ti.use_offset ? " (with relative address) " : "")
                         << (ti.is_callback ? " (callback) " : "");

            if (ti.is_callback) {
                auto insertion_pair = cb_targets.insert(std::make_pair(ti.address, &ti));
                if (!insertion_pair.second)
                    SCP_FATAL(()) << "a CB register with the same adress: 0x" << std::hex << ti.address
                                  << " was already bound!";
            } else if (dynamic_cast<gs::gs_memory<>*>(
                           gs::find_sc_obj(get_parent_object(), gs::router_if<BUSWIDTH>::parent(ti.name)))) {
                mem_targets.push_back(&ti);
            } else {
                SCP_FATAL(()) << "Expected memory model as a backing store for reg_router" << this->name();
            }
        }
    }

public:
    explicit reg_router(const sc_core::sc_module_name& nm, cci::cci_broker_handle broker = cci::cci_get_broker())
        : sc_core::sc_module(nm)
        , initiator_socket("initiator_socket", [&](std::string s) -> void { register_boundto(s); })
        , target_socket("target_socket")
        , m_broker(broker)
        , lazy_init("lazy_init", false, "Initialize the reg_router lazily (eg. during simulation rather than BEOL)")
    {
        SCP_DEBUG(()) << "reg_router constructed";

        target_socket.register_b_transport(this, &reg_router::b_transport);
        target_socket.register_transport_dbg(this, &reg_router::transport_dbg);
        target_socket.register_get_direct_mem_ptr(this, &reg_router::get_direct_mem_ptr);
        // memory model doesn't implement invalidate_direct_mem_ptr so no need to register it
        SCP_DEBUG((DMI)) << "reg_router Initializing DMI SCP reporting";
    }

    reg_router() = delete;

    reg_router(const reg_router&) = delete;

    ~reg_router() = default;

public:
    // make the sockets public for binding
    initiator_socket_type initiator_socket;
    tlm_utils::multi_passthrough_target_socket<reg_router<BUSWIDTH>, BUSWIDTH> target_socket;
    cci::cci_broker_handle m_broker;
    cci::cci_param<bool> lazy_init;

private:
    std::vector<target_info*> mem_targets;
    std::map<sc_dt::uint64, target_info*> cb_targets;
    bool initialized = false;
};
} // namespace gs

extern "C" void module_register();
#endif
