/*
 *  This file is part of GreenSocs base-components
 *  Copyright (c) 2021 Luc Michel
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

#ifndef GREENSOCS_BASE_COMPONENTS_MISC_EXCLUSIVE_MONITOR_H_
#define GREENSOCS_BASE_COMPONENTS_MISC_EXCLUSIVE_MONITOR_H_

#include <map>
#include <memory>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <greensocs/gsutils/tlm-extensions/exclusive-access.h>

/**
 * @class ExclusiveMonitor
 *
 * @brief ARM-like global exclusive monitor
 *
 * @details This component models an ARM-like global exclusive monitor. It
 * connects in front of an target and monitors accesses to it. It behaves as follows:
 *   - On an exclusive load, it internally marks the corresponding region as
 *     locked. The load is forwarded to the target.
 *   - On an exclusive store to the same region, the region is unlocked, and
 *     the store is forwarded to the target.
 *   - When an initiator perform an exclusive load while already owning a
 *     region, the region gets unlocked before the new one is locked.
 *   - A regular store will unlock all intersecting regions
 *   - If an exclusive store fails, that it, corresponds to a region which is
 *     not locked, or is locked by another initiator, or does not exactly match
 *     the store boundaries, the failure is reported into the TLM exclusive
 *     extension and the store is _not_ forwarded to the target.
 *   - DMI invalidation is performed when a region is locked.
 *   - DMI requests are intercepted and modified accordingly to match the
 *     current locking state.
 *   - DMI hints (the is_dmi_allowed() flag in transactions) is also intercepted
 *     and modified if necessary.
 */
class ExclusiveMonitor : public sc_core::sc_module {
private:
    using InitiatorId = ExclusiveAccessTlmExtension::InitiatorId;

    class Region {
    public:
        uint64_t start;
        uint64_t end;
        InitiatorId id;

        Region(const tlm::tlm_generic_payload &txn, const InitiatorId &id)
            : start(txn.get_address())
            , end(start + txn.get_data_length() - 1)
            , id(id)
        {}

        /**
         * @return true if the txn transaction matches exactly with this region,
         * i.e. start and end addresses are equal.
         */
        bool is_exact_match(const tlm::tlm_generic_payload &txn)
        {
            return (start == txn.get_address())
                && (end == start + txn.get_data_length() - 1);
        }
    };

    using RegionPtr = std::shared_ptr<Region>;

    std::map<uint64_t, RegionPtr> m_regions;
    std::map<InitiatorId, RegionPtr> m_regions_by_id;

    RegionPtr find_region(const tlm::tlm_generic_payload &txn)
    {
        uint64_t start, end;

        start = txn.get_address();
        end = start + txn.get_data_length() - 1;

        auto it = m_regions.lower_bound(start);

        if (it != m_regions.begin()) {
            it--;
        }

        while (it != m_regions.end()) {
            RegionPtr r = it->second;

            if (r->end < start) {
                it++;
                continue;
            }

            if (r->start > end) {
                break;
            }

            return r;
        }

        return nullptr;
    }

    void dmi_invalidate(RegionPtr region)
    {
        front_socket->invalidate_direct_mem_ptr(region->start, region->end);
    }

    void lock_region(const tlm::tlm_generic_payload &txn,
                     const InitiatorId &id)
    {
        RegionPtr region(std::make_shared<Region>(txn, id));

        assert(!find_region(txn));

        m_regions[region->start] = region;
        m_regions_by_id[region->id] = region;

        dmi_invalidate(region);
    }

    void unlock_region(RegionPtr region)
    {
        assert(m_regions.find(region->start) != m_regions.end());
        assert(m_regions_by_id.find(region->id) != m_regions_by_id.end());

        m_regions.erase(region->start);
        m_regions_by_id.erase(region->id);
    }

    void unlock_region_by_id(const InitiatorId &id)
    {
        if (m_regions_by_id.find(id) == m_regions_by_id.end()) {
            return;
        }

        RegionPtr region = m_regions_by_id.at(id);

        unlock_region(region);
    }

    void handle_exclusive_load(const tlm::tlm_generic_payload &txn,
                               const ExclusiveAccessTlmExtension &ext)
    {
        const InitiatorId &id = ext.get_initiator_id();
        RegionPtr region = find_region(txn);

        if (region) {
            /* Region already locked, do nothing */
            return;
        }

        /*
         * An exclusive load will unlock a previously locked region by the
         * same initiator.
         */
        unlock_region_by_id(id);

        lock_region(txn, id);
    }

    bool handle_exclusive_store(const tlm::tlm_generic_payload &txn,
                                ExclusiveAccessTlmExtension &ext)
    {
        RegionPtr region = find_region(txn);

        if (!region) {
            /* This region is not locked */
            ext.set_exclusive_store_failure();
            return false;
        }

        if (region->id != ext.get_initiator_id()) {
            /* This region is locked by another initiator */
            ext.set_exclusive_store_failure();
            return false;
        }

        if (!region->is_exact_match(txn)) {
            /* This store is not exactly aligned with the locked region */
            ext.set_exclusive_store_failure();
            return false;
        }

        ext.set_exclusive_store_success();
        unlock_region(region);

        return true;
    }

    void handle_regular_store(const tlm::tlm_generic_payload &txn)
    {
        RegionPtr region;

        /* Unlock all regions intersecting with the store */
        while ((region = find_region(txn))) {
            unlock_region(region);
        }
    }

    /*
     * Called before the actual b_transport forwarding. Handle regular and
     * exclusive stores and return true if the b_transport call must be skipped
     * completely (because of a exclusive store failure).
     */
    bool before_b_transport(const tlm::tlm_generic_payload &txn)
    {
        ExclusiveAccessTlmExtension *ext;
        bool is_store = txn.get_command() == tlm::TLM_WRITE_COMMAND;

        txn.get_extension(ext);

        if (!is_store) {
            /* Carry on with b_transport */
            return true;
        }

        if (ext) {
            /* We have an exclusive access */
            return handle_exclusive_store(txn, *ext);
        } else {
            /*
             * This is not an exclusive access. We are still interested in
             * regular stores as they will unlock a locked region.
             */
            handle_regular_store(txn);
            return true;
        }
    }

    /*
     * Called after the actual b_transport forwarding. Handles exclusive loads
     * and return true if the DMI hint must be cleared in the transaction.
     */
    bool after_b_transport(const tlm::tlm_generic_payload &txn)
    {
        ExclusiveAccessTlmExtension *ext;
        bool is_store = txn.get_command() == tlm::TLM_WRITE_COMMAND;

        txn.get_extension(ext);

        if (is_store) {
            /*
             * Already handled in before_b_transport. If we didn't return early
             * from b_transport, we know for sure we must not clear the DMI
             * hint if present.
             */
            return false;
        }

        if (!ext) {
            RegionPtr region = find_region(txn);

            /*
             * For a regular load, if the corresponding region is locked, clear
             * the DMI hint if present.
             */
            return region != nullptr;
        }

        /* We have an exclusive load */
        handle_exclusive_load(txn, *ext);

        /* We know for sure the corresponding region is locked, so clear the hint. */
        return true;
    }

    void b_transport(tlm::tlm_generic_payload &txn, sc_core::sc_time &delay)
    {
        tlm::tlm_generic_payload txn_copy;

        if (!before_b_transport(txn)) {
            /* Exclusive store failure */
            txn.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        /*
         * We keep a copy of the transaction in case the next modules in the
         * call chain mess with it.
         */
        txn_copy.deep_copy_from(txn);

        back_socket->b_transport(txn, delay);

        if (txn.get_response_status() != tlm::TLM_OK_RESPONSE) {
            /* Ignore the transaction in case the target reports a failure */
            return;
        }

        if (after_b_transport(txn_copy)) {
            txn.set_dmi_allowed(false);
        }
    }

    unsigned int transport_dbg(tlm::tlm_generic_payload &txn)
    {
        return back_socket->transport_dbg(txn);
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload &txn, tlm::tlm_dmi &dmi_data)
    {
        uint64_t txn_start;
        uint64_t fixed_start, fixed_end;
        unsigned char *fixed_ptr;
        bool ret = back_socket->get_direct_mem_ptr(txn, dmi_data);

        if (!ret) {
            /* The underlying target said no, no need to do more on our side */
            return ret;
        }

        txn_start = txn.get_address();

        fixed_start = dmi_data.get_start_address();
        fixed_end = dmi_data.get_end_address();

        auto it = m_regions.lower_bound(txn.get_address());

        if (it != m_regions.begin()) {
            it--;
        }

        for (; it != m_regions.end(); it++) {
            RegionPtr r = it->second;

            if (r->end < fixed_start) {
                /* not in the returned DMI region yet */
                continue;
            }

            if (r->start > fixed_end) {
                /* beyond the DMI region, we're done */
                break;
            }

            if ((r->start <= txn_start) && (r->end >= txn_start)) {
                /* The exclusive region intersects with the request */
                return false;
            }

            if (r->end < txn_start) {
                /* Fix the left side of the interval */
                fixed_start = r->end + 1;
            }

            if (r->start > txn_start) {
                /* Fix the right side and stop here */
                fixed_end = r->start - 1;
                break;
            }
        }

        fixed_ptr = dmi_data.get_dmi_ptr() + (fixed_start - dmi_data.get_start_address());
        dmi_data.set_dmi_ptr(fixed_ptr);
        dmi_data.set_start_address(fixed_start);
        dmi_data.set_end_address(fixed_end);

        return true;
    }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range)
    {
        front_socket->invalidate_direct_mem_ptr(start_range, end_range);
    }

public:
    tlm_utils::simple_target_socket<ExclusiveMonitor> front_socket;
    tlm_utils::simple_initiator_socket<ExclusiveMonitor> back_socket;

    ExclusiveMonitor(const sc_core::sc_module_name &name)
        : sc_core::sc_module(name)
        , front_socket("front-socket")
        , back_socket("back-socket")
    {
        front_socket.register_b_transport(this, &ExclusiveMonitor::b_transport);
        front_socket.register_transport_dbg(this, &ExclusiveMonitor::transport_dbg);
        front_socket.register_get_direct_mem_ptr(this, &ExclusiveMonitor::get_direct_mem_ptr);
        back_socket.register_invalidate_direct_mem_ptr(this, &ExclusiveMonitor::invalidate_direct_mem_ptr);
    }

    ExclusiveMonitor() = delete;
    ExclusiveMonitor(const ExclusiveMonitor&) = delete;

    virtual ~ExclusiveMonitor()
    {}
};

#endif
