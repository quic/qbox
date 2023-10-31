/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_PORTS_INITIATOR_H
#define _LIBQBOX_PORTS_INITIATOR_H

#include <functional>
#include <limits>
#include <cassert>
#include <cinttypes>

#include <tlm>

#include <libqemu-cxx/libqemu-cxx.h>

#include <greensocs/libgssync.h>

#include <scp/report.h>

#include "libqbox/qemu-instance.h"
#include "libqbox/tlm-extensions/qemu-mr-hint.h"
#include <greensocs/gsutils/tlm_sockets_buswidth.h>

class QemuInitiatorIface
{
public:
    using TlmPayload = tlm::tlm_generic_payload;

    virtual void initiator_customize_tlm_payload(TlmPayload& payload) = 0;
    virtual void initiator_tidy_tlm_payload(TlmPayload& payload) = 0;
    virtual sc_core::sc_time initiator_get_local_time() = 0;
    virtual void initiator_set_local_time(const sc_core::sc_time&) = 0;
    virtual void initiator_async_run(qemu::Cpu::AsyncJobFn job) = 0;
};

/**
 * @class QemuInitiatorSocket<>
 *
 * @brief TLM-2.0 initiator socket specialisation for QEMU AddressSpace mapping
 *
 * @details This class is used to expose a QEMU AddressSpace object as a
 *          standard TLM-2.0 initiator socket. It creates a root memory region
 *          to map the whole address space, receives I/O accesses to it and
 *          forwards them as standard TLM-2.0 transactions.
 */
template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class QemuInitiatorSocket
    : public tlm::tlm_initiator_socket<BUSWIDTH, tlm::tlm_base_protocol_types, 1, sc_core::SC_ZERO_OR_MORE_BOUND>,
      public tlm::tlm_bw_transport_if<>
{
public:
    using TlmInitiatorSocket = tlm::tlm_initiator_socket<BUSWIDTH, tlm::tlm_base_protocol_types, 1,
                                                         sc_core::SC_ZERO_OR_MORE_BOUND>;
    using TlmPayload = tlm::tlm_generic_payload;
    using MemTxResult = qemu::MemoryRegionOps::MemTxResult;
    using MemTxAttrs = qemu::MemoryRegionOps::MemTxAttrs;
    using DmiRegion = QemuInstanceDmiManager::DmiRegion;
    using DmiRegionAlias = QemuInstanceDmiManager::DmiRegionAlias;

    using DmiRegionAliasKey = uint64_t;

protected:
    QemuInstance& m_inst;
    QemuInitiatorIface& m_initiator;
    qemu::Device m_dev;
    gs::RunOnSysC m_on_sysc;

    bool m_finished = false;

    std::shared_ptr<qemu::AddressSpace> m_as;
    std::shared_ptr<qemu::MemoryListener> m_listener;

    class m_mem_obj
    {
    public:
        std::shared_ptr<qemu::MemoryRegion> m_root;
        m_mem_obj(qemu::LibQemu& inst) { m_root.reset(new qemu::MemoryRegion(inst.object_new<qemu::MemoryRegion>())); }
        m_mem_obj(std::shared_ptr<qemu::MemoryRegion> memory): m_root(std::move(memory)) {}
    };
    m_mem_obj* m_r = nullptr;

    // we use an ordered map to find and combine elements
    std::map<DmiRegionAliasKey, DmiRegionAlias::Ptr> m_dmi_aliases;
    using AliasesIterator = std::map<DmiRegionAliasKey, DmiRegionAlias::Ptr>::iterator;

    void init_payload(TlmPayload& trans, tlm::tlm_command command, uint64_t addr, uint64_t* val, unsigned int size)
    {
        trans.set_command(command);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(val));
        trans.set_data_length(size);
        trans.set_streaming_width(size);
        trans.set_byte_enable_length(0);
        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        m_initiator.initiator_customize_tlm_payload(trans);
    }

    void add_dmi_mr_alias(DmiRegionAlias::Ptr alias)
    {
        SCP_INFO(SCMOD) << "Adding " << *alias;
        qemu::MemoryRegion alias_mr = alias->get_alias_mr();
        m_r->m_root->add_subregion(alias_mr, alias->get_start());
        alias->set_installed();
    }

    void del_dmi_mr_alias(const DmiRegionAlias::Ptr alias)
    {
        if (!alias->is_installed()) {
            return;
        }
        SCP_INFO(SCMOD) << "Removing " << *alias;
        m_r->m_root->del_subregion(alias->get_alias_mr());
    }

    /**
     * @brief Request a DMI region, ask the QEMU instance DMI manager for a DMI
     * region alias for it and map it on the CPU address space.
     *
     * @param trans DMI allowed transation
     *
     * @details Ideal, the whole operation could be done on the SystemC thread for
     * simplicity. Unfortunately, QEMU misbehaves if the memory region alias
     * subregion is mapped on the root MR by another thread than the CPU
     * thread. This is related to some internal QEMU code taking different
     * paths depending on the current thread. Basically if the subregion add is
     * not done on the CPU thread, the modification won't be visible
     * immediately by the CPU so the next memory access may go through the I/O
     * path again.
     *
     * On the other hand we SHOULD do a bunch on the SystemC thread to ensure
     * validity of the DMI region and thus the alias until the point where we
     *  effectively map the alias onto the root MR. This is why we first create
     * the alias on the SystemC thread and return it to the CPU thread. Once
     * we're back to the CPU thread, we lock the DMI manager again and check
     * for the alias validity flag. If an invalidation happened in between,
     * this flag will be false and we know we can throw the entire DMI request.
     *
     * If the alias is valid after we took the lock, we can map it. If an
     * invalidation must occur, it will be done after we release the lock.
     *
     * @note We choose to protect all such areas with a mutex, allowing us to
     * process everything on the QEMU thread.
     *
     * @see QemuInstanceDmiManager for more information on why we need a global
     *      MR per DMI region.
     *
     * @note All DMI activity MUST happen from the CPU thread (from an MMIO read/write or 'safe
     * work') For 7.2 this may need to be safe aync work ????????
     *
     * @note Needs to be called with iothread locked as it will be doing several
     * updates and we dont want multiple DMI's
     *
     * @returns The DMI descriptor for the corresponding DMI region
     */
    tlm::tlm_dmi check_dmi_hint_locked(TlmPayload& trans)
    {
        assert(trans.is_dmi_allowed());
        tlm::tlm_dmi dmi_data;

        SCP_INFO(SCMOD) << "DMI request for address 0x" << std::hex << trans.get_address();

        // It is 'safer' from the SystemC perspective to  m_on_sysc.run_on_sysc([this,
        // &trans]{...}).

        if (!(*this)->get_direct_mem_ptr(trans, dmi_data)) {
            return dmi_data;
        }

        SCP_INFO(SCMOD) << "DMI Adding for address 0x" << std::hex << trans.get_address();

        // The upper limit is set within QEMU by the TBU
        // e.g. 1k small pages for ARM.
        // setting to 1/2 the size of the ARM TARGET_PAGE_SIZE,
        // Comment from QEMU code:
        /* The physical section number is ORed with a page-aligned
         * pointer to produce the iotlb entries.  Thus it should
         * never overflow into the page-aligned value.
         */
#define MAX_MAP 250

        // Current function may be called by the MMIO thread which does not hold
        // any RCU read lock. This is required in case of a memory transaction
        // commit on a TCG accelerated Qemu instance
        qemu::RcuReadLock rcu_read_lock = m_inst.get().rcu_read_lock_new();

        while (m_dmi_aliases.size() > MAX_MAP) {
            auto d = m_dmi_aliases.begin();
            std::advance(d, std::rand() % m_dmi_aliases.size());
            SCP_INFO(SCMOD) << "KULLING 0x" << std::hex << d->first;
            d = remove_alias(d);
        }

        uint64_t start = dmi_data.get_start_address();
        uint64_t end = dmi_data.get_end_address();

        if (m_dmi_aliases.size() > 0) {
            auto next = m_dmi_aliases.upper_bound(start);
            if (next != m_dmi_aliases.begin()) {
                auto prev = std::prev(next);
                DmiRegionAlias::Ptr dmi = prev->second;
                if (prev->first == start) {
                    // already have the DMI
                    assert(end <= dmi->get_end());
                    assert(dmi_data.get_dmi_ptr() == dmi->get_dmi_ptr());
                    SCP_INFO(SCMOD) << "Already have region";
                    return dmi_data;
                }
                uint64_t sz = dmi->get_size();
                if (dmi->get_end() + 1 == start && dmi->get_dmi_ptr() + sz == dmi_data.get_dmi_ptr()) {
                    SCP_INFO(SCMOD) << "Merge with previous";
                    start = dmi->get_start();
                    dmi_data.set_start_address(start);
                    dmi_data.set_dmi_ptr(dmi->get_dmi_ptr());
                    remove_alias(prev);
                }
            }
            if (next != m_dmi_aliases.end()) {
                DmiRegionAlias::Ptr dmi = next->second;
                uint64_t sz = dmi->get_size();
                if (next->first == start) {
                    // already have the DMI
                    assert(end <= dmi->get_end());
                    assert(dmi_data.get_dmi_ptr() == dmi->get_dmi_ptr());
                    SCP_INFO(SCMOD) << "Already have region(2)";
                    return dmi_data;
                }
                if (dmi->get_start() == end + 1 && dmi_data.get_dmi_ptr() + sz == dmi->get_dmi_ptr()) {
                    SCP_INFO(SCMOD) << "Merge with next";
                    end = dmi->get_end();
                    dmi_data.set_end_address(end);

                    remove_alias(next);
                }
            }
        }

        SCP_INFO(SCMOD) << "Adding DMI for range [0x" << std::hex << dmi_data.get_start_address() << "-0x" << std::hex
                        << dmi_data.get_end_address() << "]";

        DmiRegionAlias::Ptr alias = m_inst.get_dmi_manager().get_new_region_alias(dmi_data);

        m_dmi_aliases[start] = alias;
        add_dmi_mr_alias(m_dmi_aliases[start]);

        return dmi_data;
    }

    /**
     * @brief Check for the presence of the DMI hint in the transaction. If found,
     * request a DMI region, ask the QEMU instance DMI manager for a DMI region
     * alias for it and map it on the CPU address space.
     *
     * @returns The DMI descriptor for the corresponding DMI region
     */
    tlm::tlm_dmi check_dmi_hint(TlmPayload& trans)
    {
        tlm::tlm_dmi ret;
        if (!trans.is_dmi_allowed()) {
            return ret;
        }
        m_inst.get().lock_iothread();
        ret = check_dmi_hint_locked(trans);
        m_inst.get().unlock_iothread();
        return ret;
    }

    void check_qemu_mr_hint(TlmPayload& trans)
    {
        QemuMrHintTlmExtension* ext = nullptr;
        uint64_t mapping_addr;

        trans.get_extension(ext);

        if (ext == nullptr) {
            return;
        }

        qemu::MemoryRegion target_mr(ext->get_mr());

        if (target_mr.get_inst_id() != m_dev.get_inst_id()) {
            return;
        }

        mapping_addr = trans.get_address() - ext->get_offset();

        qemu::MemoryRegion mr(m_inst.get().template object_new<qemu::MemoryRegion>());

        mr.init_alias(m_dev, "mr-alias", target_mr, 0, target_mr.get_size());
        m_r->m_root->add_subregion(mr, mapping_addr);
    }

    void do_regular_access(TlmPayload& trans)
    {
        using sc_core::sc_time;

        uint64_t addr = trans.get_address();
        sc_time now = m_initiator.initiator_get_local_time();

        m_on_sysc.run_on_sysc([this, &trans, &now] { (*this)->b_transport(trans, now); });

        /*
         * Reset transaction address before dmi check (could be altered by
         * b_transport).
         */
        trans.set_address(addr);

        check_qemu_mr_hint(trans);
        check_dmi_hint(trans);

        m_initiator.initiator_set_local_time(now);
    }

    void do_debug_access(TlmPayload& trans)
    {
        m_on_sysc.run_on_sysc([this, &trans] { (*this)->transport_dbg(trans); });
    }

    MemTxResult qemu_io_access(tlm::tlm_command command, uint64_t addr, uint64_t* val, unsigned int size,
                               MemTxAttrs attrs)
    {
        TlmPayload trans;
        if (m_finished) return qemu::MemoryRegionOps::MemTxError;

        m_inst.get().unlock_iothread();

        init_payload(trans, command, addr, val, size);

        if (attrs.debug) {
            do_debug_access(trans);
        } else {
            do_regular_access(trans);
        }

        m_initiator.initiator_tidy_tlm_payload(trans);

        m_inst.get().lock_iothread();

        switch (trans.get_response_status()) {
        case tlm::TLM_OK_RESPONSE:
            return qemu::MemoryRegionOps::MemTxOK;

        case tlm::TLM_ADDRESS_ERROR_RESPONSE:
            return qemu::MemoryRegionOps::MemTxDecodeError;

        default:
            return qemu::MemoryRegionOps::MemTxError;
        }
    }

    MemTxResult qemu_io_read(uint64_t addr, uint64_t* val, unsigned int size, MemTxAttrs attrs)
    {
        return qemu_io_access(tlm::TLM_READ_COMMAND, addr, val, size, attrs);
    }

    MemTxResult qemu_io_write(uint64_t addr, uint64_t val, unsigned int size, MemTxAttrs attrs)
    {
        return qemu_io_access(tlm::TLM_WRITE_COMMAND, addr, &val, size, attrs);
    }

public:
    QemuInitiatorSocket(const char* name, QemuInitiatorIface& initiator, QemuInstance& inst)
        : TlmInitiatorSocket(name)
        , m_inst(inst)
        , m_initiator(initiator)
        , m_on_sysc(sc_core::sc_gen_unique_name("initiator_run_on_sysc"))
    {
        SCP_DEBUG(SCMOD) << "QemuInitiatorSocket constructor";
        TlmInitiatorSocket::bind(*static_cast<tlm::tlm_bw_transport_if<>*>(this));
    }

    void init(qemu::Device& dev, const char* prop)
    {
        using namespace std::placeholders;

        qemu::LibQemu& inst = m_inst.get();
        qemu::MemoryRegionOpsPtr ops;

        m_r = new m_mem_obj(inst); // oot = inst.object_new<qemu::MemoryRegion>();
        ops = inst.memory_region_ops_new();

        ops->set_read_callback(std::bind(&QemuInitiatorSocket::qemu_io_read, this, _1, _2, _3, _4));
        ops->set_write_callback(std::bind(&QemuInitiatorSocket::qemu_io_write, this, _1, _2, _3, _4));
        ops->set_max_access_size(8);

        m_r->m_root->init_io(dev, TlmInitiatorSocket::name(), std::numeric_limits<uint64_t>::max(), ops);

        dev.set_prop_link(prop, *m_r->m_root);

        m_dev = dev;
    }

    void end_of_simulation()
    {
        m_finished = true;
        cancel_all();
    }

    // This could happen during void end_of_simulation() but there is a race with other units trying
    // to pull down their DMI's
    ~QemuInitiatorSocket()
    {
        cancel_all();
        delete m_r;
        m_r = nullptr;
        //        dmimgr_unlock();
    }

    void qemu_map(qemu::MemoryListener& listener, uint64_t addr, uint64_t len)
    {
        if (m_finished) return;

        SCP_INFO(SCMOD) << "Mapping request for address [0x" << std::hex << addr << "-0x" << addr + len - 1 << "]";

        TlmPayload trans;
        uint64_t current_addr = addr;
        uint64_t temp;
        init_payload(trans, tlm::TLM_IGNORE_COMMAND, current_addr, &temp, 0);
        trans.set_dmi_allowed(true);

        while (current_addr < addr + len) {
            tlm::tlm_dmi dmi_data = check_dmi_hint_locked(trans);

            // Current addr is an absolute address while the dmi range might be relative
            // hence not necesseraly current_addr falls withing dmi_range address boundaries
            // TODO: is there a way to retrieve the dmi range block offset?
            SCP_INFO(SCMOD) << "0x" << std::hex << current_addr << " mapped [0x" << dmi_data.get_start_address()
                            << "-0x" << dmi_data.get_end_address() << "]";

            // The allocated range may not span the whole length required for mapping
            current_addr += dmi_data.get_end_address() - dmi_data.get_start_address() + 1;
            trans.set_address(current_addr);
        }

        m_initiator.initiator_tidy_tlm_payload(trans);
    }

    void init_global(qemu::Device& dev)
    {
        using namespace std::placeholders;

        qemu::LibQemu& inst = m_inst.get();
        qemu::MemoryRegionOpsPtr ops;
        ops = inst.memory_region_ops_new();

        ops->set_read_callback(std::bind(&QemuInitiatorSocket::qemu_io_read, this, _1, _2, _3, _4));
        ops->set_write_callback(std::bind(&QemuInitiatorSocket::qemu_io_write, this, _1, _2, _3, _4));
        ops->set_max_access_size(8);

        auto system_memory = inst.get_system_memory();
        system_memory->init_io(dev, TlmInitiatorSocket::name(), std::numeric_limits<uint64_t>::max(), ops);
        m_r = new m_mem_obj(std::move(system_memory));

        m_as = inst.address_space_get_system_memory();
        // System memory has been changed from container to "io", this is relevant
        // for flatview, and to reflect that we can just update the topology
        m_as->update_topology();

        m_listener = inst.memory_listener_new();
        m_listener->set_map_callback(std::bind(&QemuInitiatorSocket::qemu_map, this, _1, _2, _3));
        m_listener->register_as(m_as);

        m_dev = dev;
    }

    void cancel_all() { m_on_sysc.cancel_all(); }

    /* tlm::tlm_bw_transport_if<> */
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase,
                                               sc_core::sc_time& t)
    {
        /* Should not be reached */
        assert(false);
        return tlm::TLM_COMPLETED;
    }

    virtual AliasesIterator remove_alias(AliasesIterator it)
    {
        DmiRegionAlias::Ptr r = it->second; /*
                                             * Invalidate this region. Do not bother with
                                             * partial invalidation as it's really not worth
                                             * it. Better let the target model returns sub-DMI
                                             * regions during future accesses.
                                             */

        /*
         * Mark the whole region this alias maps to as invalid. This has
         * the effect of marking all the other aliases mapping to the same
         * region as invalid too. If a DMI request for the same region is
         * already in progress, it will have a chance to detect it is now
         * invalid before mapping it on the QEMU root MR (see
         * check_dmi_hint comment).
         */
        // r->invalidate_region();

        assert(r->is_installed());
        //        if (!r->is_installed()) {
        /*
         * The alias is not mapped onto the QEMU root MR yet. Simply
         * skip it. It will be removed from m_dmi_aliases by
         * check_dmi_hint.
         */
        //            return it++;
        //        }

        /*
         * Remove the alias from the root MR. This is enough to perform
         * required invalidations on QEMU's side in a thread-safe manner.
         */
        del_dmi_mr_alias(r);

        /*
         * Remove the alias from the collection. The DmiRegionAlias object
         * is then destructed, leading to the destruction of the DmiRegion
         * shared pointer it contains. When no more alias reference this
         * region, it is in turn destructed, effectively destroying the
         * corresponding memory region in QEMU.
         */
        return m_dmi_aliases.erase(it);
    }

private:
    void invalidate_single_range(sc_dt::uint64 start_range, sc_dt::uint64 end_range)
    {
        auto it = m_dmi_aliases.upper_bound(start_range);

        if (it != m_dmi_aliases.begin()) {
            /*
             * Start with the preceding region, as it may already cross the
             * range we must invalidate.
             */
            it--;
        }
        while (it != m_dmi_aliases.end()) {
            DmiRegionAlias::Ptr r = it->second;

            if (r->get_start() > end_range) {
                /* We've got out of the invalidation range */
                break;
            }

            if (r->get_end() < start_range) {
                /* We are not in yet */
                it++;
                continue;
            }

            it = remove_alias(it);

            SCP_INFO(SCMOD) << "Invalidated region [0x" << std::hex << r->get_start() << ", 0x" << std::hex
                            << r->get_end() << "]";
        }
    }

    std::mutex m_mutex;
    std::vector<std::pair<sc_dt::uint64, sc_dt::uint64>> m_ranges;

    void invalidate_ranges_safe_cb()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        SCP_INFO(SCMOD) << "Invalidating " << m_ranges.size() << " ranges";
        auto rit = m_ranges.begin();
        while (rit != m_ranges.end()) {
            invalidate_single_range(rit->first, rit->second);
            rit = m_ranges.erase(rit);
        }
    }

public:
    virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range)
    {
        if (m_finished) return;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            SCP_INFO(SCMOD) << "DMI invalidate [0x" << std::hex << start_range << ", 0x" << std::hex << end_range
                            << "]";
            m_ranges.push_back(std::make_pair(start_range, end_range));
        }

        m_initiator.initiator_async_run([&]() { invalidate_ranges_safe_cb(); });

        /* For 7.2 this may need to be safe aync work ???????? */
    }

    virtual void reset()
    {
        auto it = m_dmi_aliases.begin();
        while (it != m_dmi_aliases.end()) {
            DmiRegionAlias::Ptr r = it->second;
            it = remove_alias(it);
        }
    }
};

#endif
