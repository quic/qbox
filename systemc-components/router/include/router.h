/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#include <tlm-extensions/pathid_extension.h>
#include <tlm-extensions/underlying-dmi.h>
#include <cciutils.h>
#include <router_if.h>
#include <module_factory_registery.h>
#include <tlm_sockets_buswidth.h>

namespace gs {

template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class router : public sc_core::sc_module, public gs::router_if<BUSWIDTH>
{
    SCP_LOGGER_VECTOR(D);
    SCP_LOGGER(());
    SCP_LOGGER((DMI), "dmi");

    using TargetSocket = tlm::tlm_base_target_socket_b<BUSWIDTH, tlm::tlm_fw_transport_if<>,
                                                       tlm::tlm_bw_transport_if<>>;
    using InitiatorSocket = tlm::tlm_base_initiator_socket_b<BUSWIDTH, tlm::tlm_fw_transport_if<>,
                                                             tlm::tlm_bw_transport_if<>>;
    using typename gs::router_if<BUSWIDTH>::target_info;
    using initiator_socket_type = typename gs::router_if<BUSWIDTH>::template multi_passthrough_initiator_socket_spying<
        router<BUSWIDTH>>;
    using gs::router_if<BUSWIDTH>::bound_targets;

private:
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

    void register_boundto(std::string s)
    {
        s = gs::router_if<BUSWIDTH>::nameFromSocket(s);
        target_info ti = { 0 };
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
    std::string txn_tostring(target_info* ti, tlm::tlm_generic_payload& trans)
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
    // make the sockets public for binding
    initiator_socket_type initiator_socket;
    tlm_utils::multi_passthrough_target_socket<router<BUSWIDTH>, BUSWIDTH> target_socket;

private:
    std::vector<target_info> alias_targets;
    std::vector<target_info*> targets;
    std::vector<target_info*> id_targets;

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
            SCP_WARN(())("Attempt to access unknown register at offset 0x{:x}", addr);
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        stamp_txn(id, trans);
        if (!ti->chained) SCP_TRACE((D[ti->index]), ti->name) << "calling b_transport : " << txn_tostring(ti, trans);
        if (trans.get_response_status() >= tlm::TLM_INCOMPLETE_RESPONSE) {
            if (ti->use_offset) trans.set_address(addr - ti->address);
            initiator_socket[ti->index]->b_transport(trans, delay);
            if (ti->use_offset) trans.set_address(addr);
        }
        if (!ti->chained) SCP_TRACE((D[ti->index]), ti->name) << "b_transport returned : " << txn_tostring(ti, trans);
        unstamp_txn(id, trans);
    }

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        sc_dt::uint64 addr = trans.get_address();
        auto ti = decode_address(trans);
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

        tlm::tlm_dmi dmi_data_hole;
        auto ti = find_hole(addr, dmi_data_hole);

        UnderlyingDMITlmExtension* u_dmi;
        trans.get_extension(u_dmi);
        if (u_dmi) {
            SCP_DEBUG(())
            ("DMI info 0x{:x} 0x{:x} {}", dmi_data_hole.get_start_address(), dmi_data_hole.get_end_address(),
             (ti ? "mapped" : "nomap"));
            u_dmi->add_dmi(this, dmi_data_hole, (ti ? gs::tlm_dmi_ex::dmi_mapped : gs::tlm_dmi_ex::dmi_nomap));
        }

        if (!ti) {
            return false;
        }

        if (!m_dmi_mutex.try_lock()) { // if we're busy invalidating, dont grant DMI's
            return false;
        }

        if (ti->use_offset) trans.set_address(addr - ti->address);
        SCP_TRACE((D[ti->index]), ti->name) << "calling get_direct_mem_ptr : " << scp::scp_txn_tostring(trans);
        bool status = initiator_socket[ti->index]->get_direct_mem_ptr(trans, dmi_data);
        if (ti->use_offset) trans.set_address(addr);
        if (status) {
            if (ti->use_offset) {
                assert(dmi_data.get_start_address() < ti->size);
                dmi_data.set_start_address(ti->address + dmi_data.get_start_address());
                dmi_data.set_end_address(ti->address + dmi_data.get_end_address());
            }
            /* ensure we dont overspill the 'hole' we have in the address map */
            if (dmi_data.get_start_address() < dmi_data_hole.get_start_address()) {
                dmi_data.set_dmi_ptr(dmi_data.get_dmi_ptr() +
                                     (dmi_data_hole.get_start_address() - dmi_data.get_start_address()));
                dmi_data.set_start_address(dmi_data_hole.get_start_address());
            }
            if (dmi_data.get_end_address() > dmi_data_hole.get_end_address()) {
                dmi_data.set_end_address(dmi_data_hole.get_end_address());
            }
            record_dmi(id, dmi_data);
        }
        SCP_DEBUG(())
        ("Providing DMI (status {:x}) {:x} - {:x}", status, dmi_data.get_start_address(), dmi_data.get_end_address());
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

    target_info* find_hole(uint64_t addr, tlm::tlm_dmi& dmi)
    {
        uint64_t end = std::numeric_limits<uint64_t>::max();
        uint64_t start = 0;
        for (auto ti : targets) {
            if (ti->address < end && ti->address > addr) {
                end = ti->address - 1;
            }
            if ((ti->address + ti->size) > start && (ti->address + ti->size) <= addr) {
                start = ti->address + ti->size;
            }
            if (addr >= ti->address && (addr - ti->address) < ti->size) {
                if (ti->address > start) start = ti->address;
                if ((ti->address + ti->size) < end) end = ti->address + ti->size;
                SCP_DEBUG(())("Device found for address {:x} from {:x} to {:x}", addr, start, end);
                dmi.set_start_address(start);
                dmi.set_end_address(end);
                return ti;
            }
        }
        SCP_DEBUG(())("Hole for address {:x} from {:x} to {:x}", addr, start, end);
        dmi.set_start_address(start);
        dmi.set_end_address(end);
        return nullptr;
    }

protected:
    virtual void before_end_of_elaboration()
    {
        if (!lazy_init) lazy_initialize();
    }

private:
    bool initialized = false;
    void lazy_initialize()
    {
        if (initialized) return;
        initialized = true;

        for (auto& ti : bound_targets) {
            std::string name = ti.name;
            std::string src;
            if (gs::cci_get<std::string>(m_broker, ti.name + ".0", src)) {
                // deal with an alias
                if (m_broker.has_preset_value(ti.name + ".address") &&
                    m_broker.get_preset_cci_value(ti.name + ".address").is_number()) {
                    SCP_WARN((D[ti.index]), ti.name)
                    ("The configuration alias provided ({}) will be ignored as a valid address is also provided.", src);
                } else {
                    if (src[0] == '&') src = (src.erase(0, 1)) + ".target_socket";
                    if (!m_broker.has_preset_value(src + ".address")) {
                        SCP_FATAL((D[ti.index]), ti.name)
                        ("The configuration alias provided ({}) can not be found.", src);
                    }
                    name = src;
                }
            }

            ti.address = gs::cci_get<uint64_t>(m_broker, name + ".address");
            ti.size = gs::cci_get<uint64_t>(m_broker, name + ".size");
            ti.use_offset = gs::cci_get_d<bool>(m_broker, name + ".relative_addresses", true);
            ti.chained = gs::cci_get_d<bool>(m_broker, name + ".chained", false);
            ti.priority = gs::cci_get_d<uint32_t>(m_broker, name + ".priority", 0);

            SCP_INFO((D[ti.index]), ti.name)
                << "Address map " << ti.name + " at"
                << " address "
                << "0x" << std::hex << ti.address << " size "
                << "0x" << std::hex << ti.size << (ti.use_offset ? " (with relative address) " : "");
            if (ti.chained) SCP_DEBUG(())("{} is chained so debug will be suppressed", ti.name);

            for (auto tti : targets) {
                if (tti->address >= ti.address && (ti.address - tti->address) < tti->size) {
                    SCP_WARN((D[ti.index]), ti.name)("{} overlaps with {}", ti.name, tti->name);
                }
            }

            targets.push_back(&ti);

            for (std::string n : gs::sc_cci_children((ti.name + ".aliases").c_str())) {
                std::string name = ti.name + ".aliases." + n;
                uint64_t address = gs::cci_get<uint64_t>(m_broker, name + ".address");
                uint64_t size = gs::cci_get<uint64_t>(m_broker, name + ".size");
                SCP_INFO((D[ti.index]), ti.name)("Adding alias {} {:#x} (size: {})", name, address, size);
                target_info ati = ti;
                ati.address = address;
                ati.size = size;
                ati.name = name;
                alias_targets.push_back(ati);
            }
            id_targets.push_back(&ti);
        }
        for (auto& ati : alias_targets) {
            targets.push_back(&ati);
        }
        if (!targets.empty())
            std::stable_sort(targets.begin(), targets.end(), [](const target_info* first, const target_info* second) {
                return first->priority < second->priority;
            });
    }

    cci::cci_broker_handle m_broker;

public:
    cci::cci_param<bool> lazy_init;

    explicit router(const sc_core::sc_module_name& nm, cci::cci_broker_handle broker = cci::cci_get_broker())
        : sc_core::sc_module(nm)
        , initiator_socket("initiator_socket", [&](std::string s) -> void { register_boundto(s); })
        , target_socket("target_socket")
        , m_broker(broker)
        , lazy_init("lazy_init", false, "Initialize the router lazily (eg. during simulation rather than BEOL)")
    {
        SCP_DEBUG(()) << "router constructed";

        target_socket.register_b_transport(this, &router::b_transport);
        target_socket.register_transport_dbg(this, &router::transport_dbg);
        target_socket.register_get_direct_mem_ptr(this, &router::get_direct_mem_ptr);
        initiator_socket.register_invalidate_direct_mem_ptr(this, &router::invalidate_direct_mem_ptr);
        SCP_DEBUG((DMI)) << "router Initializing DMI SCP reporting";
    }

    router() = delete;

    router(const router&) = delete;

    ~router()
    {
        while (m_pathIDPool.size()) {
            delete (m_pathIDPool.back());
            m_pathIDPool.pop_back();
        }
    }

    void add_target(TargetSocket& t, const uint64_t address, uint64_t size, bool masked = true)
    {
        std::string s = gs::router_if<BUSWIDTH>::nameFromSocket(t.get_base_export().name());
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

extern "C" void module_register();
#endif
