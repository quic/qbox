/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBQBOX_DMI_MANAGER_H_
#define LIBQBOX_DMI_MANAGER_H_

#include <map>
#include <mutex>
#include <limits>
#include <cassert>
#include <memory>

#include <tlm>

#include <libqemu-cxx/libqemu-cxx.h>

#include <libgsutils.h>

#include <scp/report.h>

std::ostream& operator<<(std::ostream& os, const tlm::tlm_dmi& region);

/**
 * @class QemuInstanceDmiManager
 *
 * @brief Handles the DMI regions at the QEMU instance level
 *
 * @details This class handles the DMI regions at the level of a QEMU instance.
 *          For a given DMI region, we need to use a unique memory region
 *          (called the global memory region, in a sense that it is global to
 *          all the CPUs in the instance). Each CPU is then supposed to create
 *          an alias to this region to be able to access it. This is required
 *          to ensure QEMU sees this region as a unique piece of memory.
 *          Creating multiple regions mapping to the same host address leads
 *          QEMU into thinking that those are different data, and it won't
 *          properly invalidate corresponding TBs if CPUs do SMC (self
 *          modifying code).
 *
 * NB this class's members should be called wil holding the QEMU IOTHREAD lock
 */
class QemuInstanceDmiManager
{
public:
    /* Simple container object used to parent the memory regions we create */
    class QemuContainer : public qemu::Object
    {
    public:
        static constexpr const char* const TYPE = "container";

        QemuContainer() = default;
        QemuContainer(const QemuContainer& o) = default;
        QemuContainer(const Object& o): Object(o) {}
    };

    /**
     * @brief a DMI region
     *
     * @detail Represent a DMI region with a size and an host pointer. It also
     * embeds the QEMU memory region mapping to this host pointer. Note that it
     * does not have start and end addresses as it is totally address space
     * agnostic. Two initiators with two different views of the address space
     * can map the same DMI region.
     *
     * Note: The get_key method is used to index the map in which the regions
     * are stored. Currently, we use the host memory address itself to index
     * the map. This makes a strong assumption on the fact that two consecutive
     * DMI region requests for the same region will return the same host
     * address.  This is not clearly stated in the TLM-2.0 standard but is
     * quite reasonable to assume.
     */
    class DmiRegion
    {
    public:
        using Key = uintptr_t;
        using Ptr = std::shared_ptr<DmiRegion>;

    private:
        unsigned char* m_ptr;

        QemuContainer m_container;
        qemu::MemoryRegion m_mr;
        uint64_t m_size;

        bool m_retiring = false;

        static uint64_t size_from_tlm_dmi(const tlm::tlm_dmi& info)
        {
            uint64_t start = info.get_start_address();
            uint64_t end = info.get_end_address();
            assert(start < end);
            assert(!((start == 0) && (end == std::numeric_limits<uint64_t>::max())));
            return end - start + 1;
        }

    public:
        DmiRegion() = default;

        DmiRegion(const tlm::tlm_dmi& info, int priority, qemu::LibQemu& inst)
            : m_ptr(info.get_dmi_ptr())
            , m_container(inst.object_new_unparented<QemuContainer>())
            , m_mr(inst.object_new_unparented<qemu::MemoryRegion>())
            , m_size(size_from_tlm_dmi(info))
        {
            //          This will parent the objects
            m_mr.init_ram_ptr(m_container, "dmi", m_size, m_ptr);
            m_mr.set_priority(priority);
            SCP_INFO("DMI.Libqbox") << "Creating " << *this;
        }

        virtual ~DmiRegion() { SCP_INFO("DMI.Libqbox") << "Destroying " << *this; }

        uint64_t get_size() const { return m_size; }

        const qemu::MemoryRegion& get_mr() const { return m_mr; }
        qemu::MemoryRegion& get_mut_mr() { return m_mr; }

        Key get_key() const { return reinterpret_cast<Key>(m_ptr); }
        Key get_end() const { return get_key() + (m_size - 1); }
        unsigned char* get_ptr() const { return m_ptr; }

        static Key key_from_tlm_dmi(const tlm::tlm_dmi& info) { return reinterpret_cast<Key>(info.get_dmi_ptr()); }

        friend std::ostream& operator<<(std::ostream& os, const DmiRegion& region);
    };

    /**
     * @brief An alias to a DMI region
     *
     * @details An object of this class represents an alias to a DMI region a
     * CPU can map on its own address space. Contrary to a DmiRegion, it has a
     * start and an end address as it it requested from the point of view of an
     * initiator's address map.
     */
    class DmiRegionAlias
    {
    private:
        uint64_t m_start;
        uint64_t m_end;
        unsigned char* m_ptr;

        QemuContainer m_container;
        qemu::MemoryRegion m_alias;

        bool m_installed = false;

        friend std::ostream& operator<<(std::ostream& os, const DmiRegionAlias& alias);

    public:
        using Ptr = std::shared_ptr<DmiRegionAlias>;

        /* Construct an invalid alias */
        DmiRegionAlias() {}

        DmiRegionAlias(qemu::MemoryRegion& root, const tlm::tlm_dmi& info, qemu::LibQemu& inst)
            : m_start(info.get_start_address())
            , m_end(info.get_end_address())
            , m_ptr(info.get_dmi_ptr())
            , m_container(inst.object_new_unparented<QemuContainer>())
            , m_alias(inst.object_new_unparented<qemu::MemoryRegion>())
        {
            SCP_INFO("DMI.Libqbox") << "Creating " << *this;
            /* Offset is just the host pointer. That is the same value used as offset
             * when "overlap-adding" the RAM regions to the DMI-manager root container. */
            uint64_t offset = DmiRegion::key_from_tlm_dmi(info);
            m_alias.init_alias(m_container, "dmi-alias", root, offset, (m_end - m_start) + 1);
        }

        /* helpful for debugging */
        ~DmiRegionAlias() { SCP_INFO("DMI.Libqbox") << "Destroying " << *this; }

        DmiRegionAlias(DmiRegionAlias&&) = delete;

        uint64_t get_start() const { return m_start; }

        uint64_t get_end() const { return m_end; }

        uint64_t get_size() const { return (m_end - m_start) + 1; }

        qemu::MemoryRegion get_alias_mr() const { return m_alias; }

        unsigned char* get_dmi_ptr() const { return m_ptr; }

        /**
         * @brief Mark the alias as mapped onto QEMU root MR
         *
         * @note Must be called with the DMI manager lock held
         */
        void set_installed() { m_installed = true; }

        /**
         * @brief Return true if the alias is mapped onto QEMU root MR
         *
         * @note Must be called with the DMI manager lock held
         */
        bool is_installed() const { return m_installed; }
    };

protected:
    qemu::LibQemu& m_inst;

    QemuContainer m_root_container;
    /**
     * @brief The DMI manager maintains a "container" memory region as a root
     * where "RAM" memory regions are added to as subregions.
     * The subregions can overlap using the priority mechanism provided by QEMU.
     * These "RAM" memory regions are transparent to clients of the DMI manager,
     * and all aliases requested are built upon the root "container" memory region.
     */
    qemu::MemoryRegion m_root;

    std::mutex m_mutex;

    using DmiRegionMap = std::map<DmiRegion::Key, DmiRegion>;
    /**
     * @brief A copy of each RAM memory region is kept here. Another is stored in the
     * `m_root.m_subregions` vector. To make sure a DmiRegion gets removed, remove it
     * both from here and from the `m_root.m_subregions`.
     */
    DmiRegionMap m_regions;

    /**
     * @brief "Merge" two memory region boundaries together
     *
     * @details "Merging" means deleting the previous region, and updating tlm_dmi boundaries
     * to create a larger TlmDmi region which spans both the previous and the requested
     * boundaries.
     *
     * @return This function updates the out parameter `dmi_out` and returns the priority of the new
     * region to create.
     */
    int merge_left(const DmiRegionMap::iterator& it, tlm::tlm_dmi& dmi_out)
    {
        const DmiRegion& prev_region = it->second;
        int priority = prev_region.get_mr().get_priority();

        // We need to make sure we can actually update tlm_dmi address by
        // avoiding any underflow
        if (dmi_out.get_start_address() >= prev_region.get_size()) {
            SCP_INFO("DMI.Libqbox") << std::hex << "Merge " << dmi_out << " with previous " << prev_region;

            dmi_out.set_start_address(dmi_out.get_start_address() - prev_region.get_size());
            dmi_out.set_dmi_ptr(prev_region.get_ptr());
            // The new region is going to overlap this one that got retired, so its
            // priority should be higher
            assert(priority < std::numeric_limits<int>::max());
            priority += 1;
            // Finally, delete previous region
            m_root.del_subregion(prev_region.get_mr());
            m_regions.erase(it);
        }

        return priority;
    }

    /**
     * @brief "Merge" two memory region boundaries together
     *
     * @details "Merging" means deleting the next region, and updating tlm_dmi boundaries
     * to create a larger TlmDmi region which spans both the next and the requested
     * boundaries.
     *
     * @return This function updates the out parameter `dmi_out` and returns the priority of the
     * new region to create.
     */
    int merge_right(const DmiRegionMap::iterator& it, tlm::tlm_dmi& dmi_out)
    {
        const DmiRegion& next_region = it->second;
        int priority = next_region.get_mr().get_priority();

        // We need to make sure we can actually update tlm_dmi address by
        // avoiding any overflow
        if (dmi_out.get_end_address() <= std::numeric_limits<uint64_t>::max() - next_region.get_size()) {
            SCP_INFO("DMI.Libqbox") << std::hex << "Merge " << dmi_out << " with next " << next_region;

            dmi_out.set_end_address(dmi_out.get_end_address() + next_region.get_size());
            // The new region is going to overlap this one that got retired, so its
            // priority should be higher
            assert(priority < std::numeric_limits<int>::max());
            priority += 1;
            // Finally, delete next region
            m_root.del_subregion(next_region.get_mr());
            m_regions.erase(it);
        }

        return priority;
    }

public:
    /**
     * @brief This regions are added as subregions of the manager root memory
     * region container. They can overlap, hence we use the priority mechanism
     * provided by QEMU.
     */
    void get_region(const tlm::tlm_dmi& info)
    {
        DmiRegion::Key start = DmiRegion::key_from_tlm_dmi(info);
        uint64_t size = (info.get_end_address() - info.get_start_address()) + 1;
        uint64_t end = start + size - 1;

        tlm::tlm_dmi dmi;
        dmi.set_start_address(info.get_start_address());
        dmi.set_end_address(info.get_end_address());
        dmi.set_dmi_ptr(info.get_dmi_ptr());

        std::lock_guard<std::mutex> lock(m_mutex);

        int priority = 0;

        if (m_regions.size() > 0) {
            // Find "next", the first region starting after after `start`
            auto next = m_regions.upper_bound(start);

            // If "next" is not the first region, it means there is a region before
            if (next != m_regions.begin()) {
                auto prev = std::prev(next);
                // The previus region starts "before" or "at" start
                DmiRegion& prev_region = prev->second;
                assert(prev_region.get_key() <= start);
                auto prev_end = prev_region.get_end();

                // Check whether previos region spans the requested boundaries
                if (prev_end >= end) {
                    SCP_INFO("DMI.Libqbox") << "Already have " << prev_region << " covering " << info;
                    return;
                }

                // If previous region does not span the requested boundaries, it means it ends
                // before `end`. Here we check whether the previos region ends exactly ad `end`,
                // in which case we proceed with a merge
                if ((prev->first < start) && (prev_end + 1 == start)) {
                    priority = merge_left(prev, dmi);
                }
            }

            // At this point there is a possibility that `merge_left` deleted a DMI region, so we
            // call `upper_bound` again.
            next = m_regions.upper_bound(start);
            if (next != m_regions.end()) {
                // In this case we have a region which is starting after `start`
                DmiRegion& next_region = next->second;
                // If it starts exactly at the end of the requested region, we can merge.
                if (next_region.get_key() == end + 1) {
                    priority = merge_right(next, dmi);
                }
            }
        }

        // Another case to consider is when there is already a smaller memory region at `start`,
        // so we should erase that and create the new one with increased priority.
        auto existing_it = m_regions.find(start);
        if (existing_it != m_regions.end()) {
            DmiRegion& existing_region = existing_it->second;
            priority = existing_region.get_mr().get_priority() + 1;
            m_root.del_subregion(existing_region.get_mr());
            m_regions.erase(existing_it);
        }

        DmiRegion region = DmiRegion(dmi, priority, m_inst);
        m_root.add_subregion_overlap(region.get_mut_mr(), region.get_key());

        auto res = m_regions.emplace(region.get_key(), std::move(region));
        if (!res.second) {
            DmiRegion& existing_region = res.first->second;
            SCP_FATAL("DMI.Libqbox") << "Failed to add " << info << " for already existing " << existing_region;
        }
    }

public:
    QemuInstanceDmiManager(qemu::LibQemu& inst): m_inst(inst) { SCP_DEBUG("DMI.Libqbox") << "DmiManager Constructor"; }

    QemuInstanceDmiManager(const QemuInstanceDmiManager&) = delete;
    QemuInstanceDmiManager(QemuInstanceDmiManager&& a) = delete;

    ~QemuInstanceDmiManager() { m_root.removeSubRegions(); }

    void init()
    {
        m_root_container = m_inst.object_new_unparented<QemuContainer>();
        m_root = m_inst.object_new_unparented<qemu::MemoryRegion>();
        m_root.init(m_root_container, "dmi-manager", std::numeric_limits<uint64_t>::max());
    }

    /**
     * @brief Create a new alias for the DMI region designated by `info`
     */
    DmiRegionAlias::Ptr get_new_region_alias(const tlm::tlm_dmi& info)
    {
        get_region(info);
        return std::make_shared<DmiRegionAlias>(m_root, info, m_inst);
    }
};
#endif
