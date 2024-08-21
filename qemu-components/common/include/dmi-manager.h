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

        DmiRegion(const tlm::tlm_dmi& info, int priority, qemu::LibQemu& inst, int fd = -1)
            : m_ptr(info.get_dmi_ptr())
            , m_container(inst.object_new_unparented<QemuContainer>())
            , m_mr(inst.object_new_unparented<qemu::MemoryRegion>())
            , m_size(size_from_tlm_dmi(info))
        {
            //          This will parent the objects
            m_mr.init_ram_ptr(m_container, "dmi", m_size, m_ptr, fd);
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
    class DmiRegionAlias : public qemu::DmiRegionAliasBase
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

public:
    /**
     * @brief This regions are added as subregions of the manager root memory
     * region container. They can overlap, hence we use the priority mechanism
     * provided by QEMU.
     */
    void get_region(const tlm::tlm_dmi& info, int fd = -1)
    {
        DmiRegion::Key start = DmiRegion::key_from_tlm_dmi(info);
        uint64_t size = (info.get_end_address() - info.get_start_address()) + 1;
        uint64_t end = start + size - 1;

        tlm::tlm_dmi dmi;
        dmi.set_start_address(info.get_start_address());
        dmi.set_end_address(info.get_end_address());
        dmi.set_dmi_ptr(info.get_dmi_ptr());

        std::lock_guard<std::mutex> lock(m_mutex);

        auto existing_it = m_regions.find(start);
        if (existing_it != m_regions.end()) {
            if (existing_it->second.get_size() != size) {
                SCP_INFO("DMI.Libqbox")("Overlapping DMI regions!");
                m_root.del_subregion(existing_it->second.get_mut_mr());
                m_regions.erase(existing_it);
            } else {
                return; // Identicle region exists
            }
        }
        DmiRegion region = DmiRegion(dmi, 0, m_inst, fd);
        m_root.add_subregion(region.get_mut_mr(), region.get_key());

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
    DmiRegionAlias::Ptr get_new_region_alias(const tlm::tlm_dmi& info, int fd = -1)
    {
        get_region(info, fd);
        return std::make_shared<DmiRegionAlias>(m_root, info, m_inst);
    }
};
#endif
