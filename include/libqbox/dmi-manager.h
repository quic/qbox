/*
 *  This file is part of libqbox
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

#ifndef LIBQBOX_DMI_MANAGER_H_
#define LIBQBOX_DMI_MANAGER_H_

#include <map>
#include <mutex>
#include <limits>
#include <cassert>

#include <tlm>

#include <libqemu-cxx/libqemu-cxx.h>

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
    /*
     * Used to store the necessary information about a DMI region.
     *
     * Note: The get_key method is used to index the map in which the regions
     * are stored. Currently, we use the host memory address itself to index
     * the map. This makes a strong assumption on the fact that two consecutive
     * DMI region requests for the same region will return the same host
     * address.  This is not clearly stated in the TLM-2.0 standard but is
     * quite reasonable to assume.
     */
    struct DmiInfo {
        using Key = uintptr_t;

        uint64_t start;
        uint64_t end;
        void *ptr;
        qemu::MemoryRegion mr;

        DmiInfo() = default;

        explicit DmiInfo(const tlm::tlm_dmi &info)
            : start(info.get_start_address())
            , end(info.get_end_address())
            , ptr(info.get_dmi_ptr())
        {}

        uint64_t get_size() const
        {
            /* FIXME: possible overflow */
            assert((start != 0) || (end != std::numeric_limits<uint64_t>::max()));
            return end - start + 1;
        }

        Key get_key() const
        {
            return reinterpret_cast<Key>(ptr);
        }
    };

    /* Simple container object used to parent the memory regions we create */
    class QemuContainer : public qemu::Object {
    public:
        static constexpr const char * const TYPE = "container";

        QemuContainer() = default;
        QemuContainer(const QemuContainer &o) = default;
        QemuContainer(const Object &o) : Object(o) {}
    };

protected:
    qemu::LibQemu &m_inst;
    QemuContainer m_mr_container; /* Used for MR ref counting in QEMU */
    std::mutex m_mutex;

    std::map<DmiInfo::Key, DmiInfo> m_dmis;

    void create_global_mr(DmiInfo &info)
    {
        assert(!info.mr.valid());

        info.mr = m_inst.object_new<qemu::MemoryRegion>();
        info.mr.init_ram_ptr(m_mr_container, "dmi", info.get_size(), info.ptr);

        m_dmis[info.get_key()] = info;
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
        , m_mr_container(std::move(a.m_mr_container))
        , m_dmis(std::move(a.m_dmis))
    {}

    void init()
    {
        m_mr_container = m_inst.object_new<QemuContainer>();
    }

    /**
     * @brief Fill DMI info with the corresponding global memory region
     *
     * @details Fill the DMI info `info' with the global memory region matching
     * the rest of `info'. If the global MR does not exist yet, it is created.
     * Otherwise the already existing one is returned.
     *
     * @param[in,out] info The DMI info to use and to fill with the global MR
     */
    void get_global_mr(DmiInfo &info)
    {
        if (m_dmis.find(info.get_key()) == m_dmis.end()) {
            create_global_mr(info);
        }

        info = m_dmis.at(info.get_key());
        assert(info.mr.valid());
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
    using DmiInfo = QemuInstanceDmiManager::DmiInfo;

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
     * @see QemuInstanceDmiManager::get_global_mr
     */
    void get_global_mr(DmiInfo &info)
    {
        m_inst.get_global_mr(info);
    }
};

#endif
