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

#ifndef _GREENSOCS_BASE_COMPONENTS_ROUTER_H
#define _GREENSOCS_BASE_COMPONENTS_ROUTER_H

#include <cinttypes>
#include <vector>

#define THREAD_SAFE true
#if THREAD_SAFE == true
#include <mutex>
#endif

#include <iomanip>

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>
#include <scp/helpers.h>

#include <list>

#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

#include <greensocs/base-components/pathid_extension.h>
#include <greensocs/gsutils/cciutils.h>

namespace gs {

template <unsigned int BUSWIDTH = 32>
class Router : public sc_core::sc_module
{
    using TargetSocket = tlm::tlm_base_target_socket_b<BUSWIDTH, tlm::tlm_fw_transport_if<>,
                                                       tlm::tlm_bw_transport_if<>>;
    using InitiatorSocket = tlm::tlm_base_initiator_socket_b<BUSWIDTH, tlm::tlm_fw_transport_if<>,
                                                             tlm::tlm_bw_transport_if<>>;
    SCP_LOGGER_VECTOR(D);
    SCP_LOGGER(());
    SCP_LOGGER((DMI), "dmi");

private:
    template <typename MOD>
    class multi_passthrough_initiator_socket_spying
        : public tlm_utils::multi_passthrough_initiator_socket<MOD, BUSWIDTH>
    {
        using typename tlm_utils::multi_passthrough_initiator_socket<MOD, BUSWIDTH>::base_target_socket_type;

        const std::function<void(std::string)> register_cb;

    public:
        multi_passthrough_initiator_socket_spying(const char* name, const std::function<void(std::string)>& f)
            : tlm_utils::multi_passthrough_initiator_socket<MOD, BUSWIDTH>::multi_passthrough_initiator_socket(name)
            , register_cb(f)
        {
        }

        void bind(base_target_socket_type& socket)
        {
            tlm_utils::multi_passthrough_initiator_socket<MOD, BUSWIDTH>::bind(socket);
            register_cb(socket.get_base_export().name());
        }
    };

    struct target_info {
        size_t index;
        std::string name;
        sc_dt::uint64 address;
        sc_dt::uint64 size;
        bool use_offset;
        bool is_callback;
        bool chained;
        std::string shortname;
    };
    std::string parent(std::string name) { return name.substr(0, name.find_last_of('.')); }

    std::mutex m_dmi_mutex;
    struct dmi_info {
        std::set<int> initiators;
        tlm::tlm_dmi dmi;
        dmi_info(tlm::tlm_dmi& _dmi) { dmi = _dmi; }
    };
    std::map<uint64_t, dmi_info> m_dmi_info_map;
    dmi_info* in_dmi_cache(tlm::tlm_dmi& dmi)
    {
        auto it = m_dmi_info_map.find(dmi.get_start_address());
        if (it != m_dmi_info_map.end()) {
            if (it->second.dmi.get_end_address() != dmi.get_end_address()) {
                SCP_FATAL((DMI)) << "Can't handle that";
            }
            return &(it->second);
        }
        auto insit = m_dmi_info_map.insert({ dmi.get_start_address(), dmi_info(dmi) });
        return &(insit.first->second);
    }
    void record_dmi(int id, tlm::tlm_dmi& dmi)
    {
        auto it = m_dmi_info_map.find(dmi.get_start_address());
        if (it != m_dmi_info_map.end()) {
            if (it->second.dmi.get_end_address() != dmi.get_end_address()) {
                SCP_WARN((DMI)) << "A new DMI overlaps with an old one, invalidating the old one";
                invalidate_direct_mem_ptr_ts(0, dmi.get_start_address(),
                                             dmi.get_end_address()); // id will be ignored
            }
        }

        dmi_info* dinfo = in_dmi_cache(dmi);
        dinfo->initiators.insert(id);
    }

    /* NB use the EXPORT name, so as not to be hassled by the _port_0*/
    std::string nameFromSocket(std::string s) { return s; }
    void register_boundto(std::string s)
    {
        s = nameFromSocket(s);
        struct target_info ti = { 0 };
        ti.name = s;
        ti.index = bound_targets.size();
        SCP_LOGGER_VECTOR_PUSH_BACK(D, ti.name);
        SCP_DEBUG((D[ti.index])) << "Connecting : " << ti.name;
        ti.chained = false;
        auto tmp = name();
        int i;
        for (i = 0; i < s.length(); i++)
            if (s[i] != tmp[i]) break;
        ti.shortname = s.substr(i);

        bound_targets.push_back(ti);
    }
    std::map<uint64_t, std::string> name_map;
    std::string txn_tostring(struct target_info* ti, tlm::tlm_generic_payload& trans)
    {
        std::stringstream info;
        const char* cmd = "UNKOWN";
        switch (trans.get_command()) {
        case tlm::TLM_IGNORE_COMMAND:
            info << "IGNORE ";
            break;
        case tlm::TLM_WRITE_COMMAND:
            info << "WRITE ";
            break;
        case tlm::TLM_READ_COMMAND:
            info << "READ ";
            break;
        }

        if (ti->size > trans.get_data_length()) {
            sc_dt::uint64 addr = trans.get_address();
            sc_dt::uint64 len = trans.get_data_length();

            bool found = false;
            for (auto cbti : cb_targets) {
                if (addr >= cbti->address && (addr - cbti->address) < cbti->size) {
                    info << cbti->shortname << " ";
                    found = true;
                }
            }
            if (!found) {
                if (name_map.empty()) {
                    for (auto nn : gs::sc_cci_children(parent(name()).c_str())) {
                        std::string fn = parent(name()) + "." + nn;
                        uint64_t n_addr = get_val<uint64_t>(fn + ".target_socket.address", 0);
                        uint64_t n_size = get_val<uint64_t>(fn + ".target_socket.size", 0);
                        uint64_t n_num = get_val<uint64_t>(fn + ".number", 0);
                        if (!n_num) { // handle vectors?
                            name_map[n_addr] = nn + " " + name_map[n_addr];
                        }
                    }
                }
                if (name_map.count(addr)) {
                    info << name_map[addr];
                }
            }
        }
        info << " address:"
             << "0x" << std::hex << trans.get_address();
        info << " len:" << trans.get_data_length();
        unsigned char* ptr = trans.get_data_ptr();
        if ((trans.get_command() == tlm::TLM_READ_COMMAND && trans.get_response_status() == tlm::TLM_OK_RESPONSE) ||
            (trans.get_command() == tlm::TLM_WRITE_COMMAND &&
             trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE)) {
            info << " data:0x";
            for (int i = trans.get_data_length(); i; i--) {
                info << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(ptr[i - 1]);
            }
        }
        info << " " << trans.get_response_string() << " ";
        for (int i = 0; i < tlm::max_num_extensions(); i++) {
            if (trans.get_extension(i)) {
                info << " extn:" << i;
            }
        }
        return info.str();
    }

public:
    void rename_last(std::string s)
    {
        target_info& ti = bound_targets.back();
        ti.name = s;
    }
    // make the sockets public for binding
    typedef multi_passthrough_initiator_socket_spying<Router<BUSWIDTH>> initiator_socket_type;
    initiator_socket_type initiator_socket;
    tlm_utils::multi_passthrough_target_socket<Router<BUSWIDTH>> target_socket;

private:
    std::vector<target_info> bound_targets;
    std::vector<target_info> alias_targets;
    std::vector<target_info*> targets;
    std::vector<target_info*> cb_targets;
    std::vector<target_info*> id_targets;
    std::vector<target_info*> dynamic_targets;

    std::vector<PathIDExtension*> m_pathIDPool; // at most one per thread!
#if THREAD_SAFE == true
    std::mutex m_pool_mutex;
#endif
    void stamp_txn(int id, tlm::tlm_generic_payload& txn)
    {
        PathIDExtension* ext = nullptr;
        txn.get_extension(ext);
        if (ext == nullptr) {
#if THREAD_SAFE == true
            std::lock_guard<std::mutex> l(m_pool_mutex);
#endif
            if (m_pathIDPool.size() == 0) {
                ext = new PathIDExtension();
            } else {
                ext = m_pathIDPool.back();
                m_pathIDPool.pop_back();
            }
            txn.set_extension(ext);
        }
        ext->push_back(id);
    }
    void unstamp_txn(int id, tlm::tlm_generic_payload& txn)
    {
        PathIDExtension* ext;
        txn.get_extension(ext);
        assert(ext);
        assert(ext->back() == id);
        ext->pop_back();
        if (ext->size() == 0) {
#if THREAD_SAFE == true
            std::lock_guard<std::mutex> l(m_pool_mutex);
#endif
            txn.clear_extension(ext);
            m_pathIDPool.push_back(ext);
        }
    }

    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        bool found = false;
        sc_dt::uint64 addr = trans.get_address();
        auto ti = decode_address(trans);
        if (!ti) {
            for (auto dti : dynamic_targets) {
                initiator_socket[dti->index]->b_transport(trans, delay);
                if (trans.get_response_status() == tlm::TLM_OK_RESPONSE) {
                    return;
                }
            }
        }
        if (!ti) {
            SCP_WARN(())("Attempt to access unknown register at offset 0x{:x}", addr);
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        stamp_txn(id, trans);
        if (!ti->chained) SCP_TRACE((D[ti->index]), ti->name) << "calling b_transport : " << txn_tostring(ti, trans);
        do_callbacks(trans, delay);
        if (trans.get_response_status() >= tlm::TLM_INCOMPLETE_RESPONSE) {
            if (ti->use_offset) trans.set_address(addr - ti->address);
            initiator_socket[ti->index]->b_transport(trans, delay);
            if (ti->use_offset) trans.set_address(addr);
        }
        if (trans.get_response_status() >= tlm::TLM_OK_RESPONSE) {
            do_callbacks(trans, delay);
        }
        if (!ti->chained) SCP_TRACE((D[ti->index]), ti->name) << "b_transport returned : " << txn_tostring(ti, trans);
        unstamp_txn(id, trans);
        if (cb_targets.size() != 0) { /* If ANY callbacks are registered we deny ALL DMI access ! */
            trans.set_dmi_allowed(false);
        }
    }

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        sc_dt::uint64 addr = trans.get_address();
        auto ti = decode_address(trans);
        if (!ti) {
            for (auto dti : dynamic_targets) {
                unsigned int ret = initiator_socket[dti->index]->transport_dbg(trans);
                if (trans.get_response_status() == tlm::TLM_OK_RESPONSE) {
                    return ret;
                }
            }
        }
        if (!ti) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return 0;
        }

        if (ti->use_offset) trans.set_address(addr - ti->address);
        SCP_TRACE((D[ti->index]), ti->name) << "calling dbg_transport : " << scp::scp_txn_tostring(trans);
        unsigned int ret = initiator_socket[ti->index]->transport_dbg(trans);
        if (ti->use_offset) trans.set_address(addr);
        return ret;
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        sc_dt::uint64 addr = trans.get_address();
        auto ti = decode_address(trans);
        if (!ti) {
            for (auto dti : dynamic_targets) {
                unsigned int ret = initiator_socket[dti->index]->get_direct_mem_ptr(trans, dmi_data);
                if (ret) {
                    return ret;
                }
            }
        }
        if (!ti) {
            return false;
        }

        if (ti->use_offset) trans.set_address(addr - ti->address);

        if (!m_dmi_mutex.try_lock()) { // if we're busy invalidating, dont grant DMI's
            return false;
        }

        SCP_TRACE((D[ti->index]), ti->name) << "calling get_direct_mem_ptr : " << scp::scp_txn_tostring(trans);
        bool status = initiator_socket[ti->index]->get_direct_mem_ptr(trans, dmi_data);
        if (status) {
            if (ti->use_offset) {
                assert(dmi_data.get_start_address() < ti->size);
                dmi_data.set_start_address(ti->address + dmi_data.get_start_address());
                dmi_data.set_end_address(ti->address + dmi_data.get_end_address());
                trans.set_address(addr);
            }
            record_dmi(id, dmi_data);
        }
        m_dmi_mutex.unlock();
        return status;
    }

    void invalidate_direct_mem_ptr(int id, sc_dt::uint64 start, sc_dt::uint64 end)
    {
        if (id_targets[id]->use_offset) {
            start = id_targets[id]->address + start;
            end = id_targets[id]->address + end;
        }
        std::lock_guard<std::mutex> lock(m_dmi_mutex);
        invalidate_direct_mem_ptr_ts(id, start, end);
    }

    void invalidate_direct_mem_ptr_ts(int id, sc_dt::uint64 start, sc_dt::uint64 end)
    {
        auto it = m_dmi_info_map.upper_bound(start);

        if (it != m_dmi_info_map.begin()) {
            /*
             * Start with the preceding region, as it may already cross the
             * range we must invalidate.
             */
            it--;
        }

        while (it != m_dmi_info_map.end()) {
            tlm::tlm_dmi& r = it->second.dmi;

            if (r.get_start_address() > end) {
                /* We've got out of the invalidation range */
                break;
            }

            if (r.get_end_address() < start) {
                /* We are not in yet */
                it++;
                continue;
            }
            for (auto t : it->second.initiators) {
                SCP_INFO((DMI)) << "Invalidating initiator " << t << " [0x" << std::hex
                                << it->second.dmi.get_start_address() << " - 0x" << it->second.dmi.get_end_address()
                                << "]";
                target_socket[t]->invalidate_direct_mem_ptr(it->second.dmi.get_start_address(),
                                                            it->second.dmi.get_end_address());
            }
            it = m_dmi_info_map.erase(it);
        }
    }

    void do_callbacks(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        sc_dt::uint64 addr = trans.get_address();
        sc_dt::uint64 len = trans.get_data_length();

        for (auto ti : cb_targets) {
            if (addr >= ti->address && (addr - ti->address) < ti->size) {
                if (ti->use_offset) trans.set_address(addr - ti->address);
                initiator_socket[ti->index]->b_transport(trans, delay);
                if (ti->use_offset) trans.set_address(addr);
                if (trans.get_response_status() <= tlm::TLM_GENERIC_ERROR_RESPONSE) break;
            }
        }
    }

    target_info* decode_address(tlm::tlm_generic_payload& trans)
    {
        lazy_initialize();

        sc_dt::uint64 addr = trans.get_address();
        sc_dt::uint64 len = trans.get_data_length();

        for (auto ti : targets) {
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

private:
    template <class TYPE>
    inline TYPE get_val(std::string s, bool use_default, TYPE default_val)
    {
        TYPE ret;

        auto h = m_broker.get_param_handle(s);
        cci::cci_value v;
        if (h.is_valid()) {
            v = h.get_cci_value();
        } else {
            if (!m_broker.has_preset_value(s)) {
                if (use_default) {
                    m_broker.lock_preset_value(s);
                    return default_val;
                } else {
                    SCP_FATAL(()) << "Can't find " << s << " in " << m_broker.name();
                }
            }

            v = m_broker.get_preset_cci_value(s);
        }
        if (v.is_uint64())
            ret = v.get_uint64();
        else if (v.is_int64())
            ret = v.get_int64();
        else if (v.is_bool())
            ret = v.get_bool();
        else if (use_default)
            ret = default_val;
        else {
            SCP_FATAL(())("Unknown type for {} : {}", s, v.to_json());
        }

        m_broker.lock_preset_value(s);
        m_broker.ignore_unconsumed_preset_values(
            [&s](const std::pair<std::string, cci::cci_value>& iv) -> bool { return iv.first == (s); });
        return ret;
    }
    template <class TYPE>
    inline TYPE get_val(std::string s, TYPE default_val)
    {
        return get_val<TYPE>(s, true, default_val);
    }
    template <class TYPE>
    inline TYPE get_val(std::string s)
    {
        return get_val<TYPE>(s, false, 0);
    }
    bool initialized = false;
    void lazy_initialize()
    {
        if (initialized) return;
        initialized = true;

        for (auto& ti : bound_targets) {
            if (get_val<bool>(ti.name + ".dynamic", false) == true) {
                dynamic_targets.push_back(&ti);
                continue;
            }
            ti.address = get_val<uint64_t>(ti.name + ".address");
            ti.size = get_val<uint64_t>(ti.name + ".size");
            ti.use_offset = get_val<bool>(ti.name + ".relative_addresses", true);
            ti.is_callback = get_val<bool>(ti.name + ".is_callback", false);
            ti.chained = get_val<bool>(ti.name + ".chained", false);

            SCP_INFO((D[ti.index]), ti.name)
                << "Address map " << ti.name + " at"
                << " address "
                << "0x" << std::hex << ti.address << " size "
                << "0x" << std::hex << ti.size << (ti.use_offset ? " (with relative address) " : "")
                << (ti.is_callback ? " (callback) " : "");
            if (ti.chained) SCP_DEBUG(())("{} is chained so debug will be suppressed", ti.name);

            if (ti.is_callback) {
                cb_targets.push_back(&ti);
            } else {
                targets.push_back(&ti);

                for (std::string n : gs::sc_cci_children((ti.name + ".aliases").c_str())) {
                    std::string name = ti.name + ".aliases." + n;
                    uint64_t address = get_val<uint64_t>(name + ".address");
                    uint64_t size = get_val<uint64_t>(name + ".size");
                    SCP_INFO((D[ti.index]), ti.name)("Adding alias {} {:#x} (size: {})", name, address, size);
                    struct target_info ati = ti;
                    ati.address = address;
                    ati.size = size;
                    ati.name = name;
                    alias_targets.push_back(ati);
                }
                id_targets.push_back(&ti);
            }
        }
        for (auto& ati : alias_targets) {
            targets.push_back(&ati);
        }
    }

    cci::cci_broker_handle m_broker;

public:
    cci::cci_param<bool> thread_safe;
    cci::cci_param<bool> lazy_init;

    explicit Router(const sc_core::sc_module_name& nm, cci::cci_broker_handle broker = cci::cci_get_broker())
        : sc_core::sc_module(nm)
        , initiator_socket("initiator_socket", [&](std::string s) -> void { register_boundto(s); })
        , target_socket("target_socket")
        , m_broker(broker)
        , thread_safe("thread_safe", THREAD_SAFE, "Is this model thread safe")
        , lazy_init("lazy_init", false, "Initialize the router lazily (eg. during simulation rather than BEOL)")
    {
        SCP_DEBUG(()) << "Router constructed";

        target_socket.register_b_transport(this, &Router::b_transport);
        target_socket.register_transport_dbg(this, &Router::transport_dbg);
        target_socket.register_get_direct_mem_ptr(this, &Router::get_direct_mem_ptr);
        initiator_socket.register_invalidate_direct_mem_ptr(this, &Router::invalidate_direct_mem_ptr);
        SCP_DEBUG((DMI)) << "Router Initializing DMI SCP reporting";
    }

    Router() = delete;

    Router(const Router&) = delete;

    ~Router()
    {
        while (m_pathIDPool.size()) {
            delete (m_pathIDPool.back());
            m_pathIDPool.pop_back();
        }
    }

    void add_target(TargetSocket& t, const uint64_t address, uint64_t size, bool masked = true)
    {
        std::string s = nameFromSocket(t.get_base_export().name());
        if (!m_broker.has_preset_value(s + ".address")) {
            m_broker.set_preset_cci_value(s + ".address", cci::cci_value(address));
        }
        if (!m_broker.has_preset_value(s + ".size")) {
            m_broker.set_preset_cci_value(s + ".size", cci::cci_value(size));
        }
        if (!m_broker.has_preset_value(s + ".relative_addresses")) {
            m_broker.set_preset_cci_value(s + ".relative_addresses", cci::cci_value(masked));
        }
        initiator_socket.bind(t);
    }

    virtual void add_initiator(InitiatorSocket& i)
    {
        // hand bind the port/exports as we are using base classes
        (i.get_base_port())(target_socket.get_base_interface());
        (target_socket.get_base_port())(i.get_base_interface());
    }
};
} // namespace gs
#endif
