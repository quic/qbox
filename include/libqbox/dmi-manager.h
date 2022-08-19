/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs
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

#ifndef LIBQBOX_DMI_MANAGER_H_
#define LIBQBOX_DMI_MANAGER_H_

#include <map>
#include <mutex>
#include <limits>
#include <cassert>
#include <memory>

#include <tlm>

#include <libqemu-cxx/libqemu-cxx.h>

#include <greensocs/libgsutils.h>

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
 */
class QemuInstanceDmiManager
{
public:
    /* Simple container object used to parent the memory regions we create */
    class QemuContainer : public qemu::Object {
    public:
        static constexpr const char * const TYPE = "container";

        QemuContainer() = default;
        QemuContainer(const QemuContainer &o) = default;
        QemuContainer(const Object &o) : Object(o) {}
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
    class DmiRegion {
    public:
        using Key = uintptr_t;
        using Ptr = std::shared_ptr<DmiRegion>;

    private:
        void *m_ptr;

        bool m_valid;
        QemuContainer m_container;
        qemu::MemoryRegion m_mr;
        uint64_t m_size;

        void set_size(const tlm::tlm_dmi &info)
        {
            uint64_t start, end;

            start = info.get_start_address();
            end = info.get_end_address();

            if ((start == 0) && (end == std::numeric_limits<uint64_t>::max())) {
                /* TODO if needed */
                assert(false);
            } else {
                m_size = info.get_end_address() - info.get_start_address() + 1;
            }
        }

    public:
        DmiRegion() = default;

        DmiRegion(const tlm::tlm_dmi &info, qemu::LibQemu &inst)
            : m_ptr(info.get_dmi_ptr())
            , m_valid(true)
            , m_container(inst.object_new_unparented<QemuContainer>())
            , m_mr(inst.object_new_unparented<qemu::MemoryRegion>())
        {
            set_size(info);
//          This will parent the objects
            m_mr.init_ram_ptr(m_container, "dmi", get_size(), m_ptr);
        }

        virtual ~DmiRegion()
        {
            GS_LOG("Destroying DMI region for host ptr %p", m_ptr);
        }

        uint64_t get_size() const
        {
            return m_size;
        }

        qemu::MemoryRegion get_mr()
        {
            return m_mr;
        }

        Key get_key() const
        {
            return reinterpret_cast<Key>(m_ptr);
        }

        static Key key_from_tlm_dmi(const tlm::tlm_dmi &info)
        {
            return reinterpret_cast<Key>(info.get_dmi_ptr());
        }

        bool is_valid() const
        {
            return m_valid;
        }

        void invalidate()
        {
            m_valid = false;
        }
    };

    /**
     * @brief An alias to a DMI region
     *
     * @detail An object of this class represents an alias to a DMI region a
     * CPU can map on its own address space. Contrary to a DmiRegion, it has a
     * start and an end address as it it requested from the point of view of an
     * initiator's address map.
     *
     * It embeds a shared pointer of the underlying DMI region. The DMI region
     * get destroyed once all aliases referencing it have been destroyed.
     */
    class DmiRegionAlias {
    private:
        DmiRegion::Ptr m_region;

        uint64_t m_start;
        uint64_t m_end;

        QemuContainer m_container;
        qemu::MemoryRegion m_alias;

        bool m_installed = false;

    public:
        using Ptr = std::shared_ptr<DmiRegionAlias>;

        /* Construct an invalid alias */
        DmiRegionAlias()
        {}

        DmiRegionAlias(DmiRegion::Ptr region, const tlm::tlm_dmi &info, qemu::LibQemu &inst)
            : m_region(region)
            , m_start(info.get_start_address())
            , m_end(info.get_end_address())
            , m_container(inst.object_new_unparented<QemuContainer>())
            , m_alias(inst.object_new_unparented<qemu::MemoryRegion>())
        {
//          This will parent the objects.
            m_alias.init_alias(m_container, "dmi-alias",
                               m_region->get_mr(), 0,
                               m_region->get_size());
        }

        /* helpful for debugging */
        ~DmiRegionAlias()
        {
            GS_LOG("Destroying DMI region Alias");
        }

        DmiRegionAlias(DmiRegionAlias &&)=delete;

        uint64_t get_start() const
        {
            return m_start;
        }

        uint64_t get_end() const
        {
            return m_end;
        }

        uint64_t get_size() const
        {
            return m_region->get_size();
        }

        qemu::MemoryRegion get_alias_mr() const
        {
            return m_alias;
        }

        /**
         * @brief Return true if the alias and its underlying DMI region are valid
         *
         * @note Must be called with the DMI manager lock held
         */
        bool is_valid() const
        {
            return m_region && m_region->is_valid();
        }

        /**
         * @brief Invalidate the underlying DMI region
         *
         * @note Must be called with the DMI manager lock held
         */
        void invalidate_region()
        {
            m_region->invalidate();
        }

        /**
         * @brief Mark the alias as mapped onto QEMU root MR
         *
         * @note Must be called with the DMI manager lock held
         */
        void set_installed()
        {
            m_installed = true;
        }

        /**
         * @brief Return true if the alias is mapped onto QEMU root MR
         *
         * @note Must be called with the DMI manager lock held
         */
        bool is_installed() const
        {
            return m_installed;
        }
    };

protected:
    qemu::LibQemu &m_inst;
    std::mutex m_mutex;

    /*
     * Keep a weak pointer on the DMI region so that they get destroyed once no
     * alias reference it anymore.
     */
    std::map< DmiRegion::Key, std::weak_ptr<DmiRegion> > m_regions;

    DmiRegion::Ptr create_region(const tlm::tlm_dmi &info)
    {
        DmiRegion::Ptr ret = std::make_shared<DmiRegion>(info, m_inst);

        return ret;
    }

    DmiRegion::Ptr get_region(const tlm::tlm_dmi &info)
    {
        DmiRegion::Key key = DmiRegion::key_from_tlm_dmi(info);
        DmiRegion::Ptr ret;

        ret = m_regions[key].lock();

        if (!ret) {
            /*
             * The corresponding region either does not exist or has been
             * destroyed. Create a new one.
             */
            ret = create_region(info);
            m_regions[key] = ret;
        }

        return ret;
    }

public:
    QemuInstanceDmiManager(qemu::LibQemu &inst)
        : m_inst(inst)
    {}

    QemuInstanceDmiManager(const QemuInstanceDmiManager &) = delete;

    /*
     * The move constructor is not locked as we do not expect concurrent code
     * to use the instance directly, but through the
     * LockedQemuInstanceDmiManager class.
     */
    QemuInstanceDmiManager(QemuInstanceDmiManager &&a)
        : m_inst(a.m_inst)
        , m_regions(std::move(a.m_regions))
    {}

    /**
     * @brief Create a new alias for the DMI region designated by `info`
     */
    DmiRegionAlias::Ptr get_new_region_alias(const tlm::tlm_dmi& info)
    {
        DmiRegion::Ptr region(get_region(info));

        return std::make_shared<DmiRegionAlias>(region, info, m_inst);
    }

    friend class LockedQemuInstanceDmiManager;
};

/**
 * @class LockedQemuInstanceDmiManager
 *
 * @brief A locked QemuInstanceDmiManager
 *
 * @details This class is a wrapper around QemuInstanceDmiManager that ensure
 * safe accesses to it. As long as an instance of this class is live, the
 * underlying QemuInstanceDmiManager is locked. It gets unlocked once the
 * object goes out of scope.
 */
class LockedQemuInstanceDmiManager
{
public:
    using DmiRegion = QemuInstanceDmiManager::DmiRegion;

protected:
    QemuInstanceDmiManager &m_inst;
    std::unique_lock<std::mutex> m_lock;

public:
    LockedQemuInstanceDmiManager(QemuInstanceDmiManager &inst)
        : m_inst(inst)
        , m_lock(inst.m_mutex)
    {}

    LockedQemuInstanceDmiManager(const LockedQemuInstanceDmiManager &) = delete;
    LockedQemuInstanceDmiManager(LockedQemuInstanceDmiManager &&) = default;

    /**
     * @see QemuInstanceDmiManager::get_new_region_alias
     */
    QemuInstanceDmiManager::DmiRegionAlias::Ptr get_new_region_alias(const tlm::tlm_dmi &info)
    {
        return m_inst.get_new_region_alias(info);
    }
};

#endif
