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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
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

class QemuInitiatorIface
{
public:
    using TlmPayload = tlm::tlm_generic_payload;

    virtual void initiator_customize_tlm_payload(TlmPayload& payload) = 0;
    virtual void initiator_tidy_tlm_payload(TlmPayload& payload) = 0;
    virtual sc_core::sc_time initiator_get_local_time() = 0;
    virtual void initiator_set_local_time(const sc_core::sc_time&) = 0;
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
template <unsigned int BUSWIDTH = 32>
class QemuInitiatorSocket : public tlm::tlm_initiator_socket<BUSWIDTH, tlm::tlm_base_protocol_types,
                                                             1, sc_core::SC_ZERO_OR_MORE_BOUND>,
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

    tlm::tlm_dmi m_dmi_data;
    bool m_finished = false;

    class m_mem_obj
    {
    public:
        qemu::MemoryRegion m_root;
        m_mem_obj(qemu::LibQemu& inst) { m_root = inst.object_new<qemu::MemoryRegion>(); }
    };
    m_mem_obj* m_r = nullptr;

    // we use an ordered map to find and combine elements
    std::map<DmiRegionAliasKey, DmiRegionAlias::Ptr> m_dmi_aliases;
    using AliasesIterator = std::map<DmiRegionAliasKey, DmiRegionAlias::Ptr>::iterator;

    void init_payload(TlmPayload& trans, tlm::tlm_command command, uint64_t addr, uint64_t* val,
                      unsigned int size) {
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

    void add_dmi_mr_alias(DmiRegionAlias::Ptr alias) {
        SCP_INFO(SCMOD) << "Adding DMI alias for region [ 0x" << std::hex << alias->get_start()
                        << ", 0x" << std::hex << alias->get_end() << "]";

        qemu::MemoryRegion alias_mr = alias->get_alias_mr();
        m_r->m_root.add_subregion(alias_mr, alias->get_start());

        alias->set_installed();
    }

    void del_dmi_mr_alias(const DmiRegionAlias::Ptr alias) {
        if (!alias->is_installed()) {
            return;
        }

        SCP_INFO(SCMOD) << "Removing DMI alias for region [ 0x" << std::hex << alias->get_start()
                        << ", 0x" << std::hex << alias->get_end() << "]";

        m_r->m_root.del_subregion(alias->get_alias_mr());
    }

    /*
     * Check for the presence of the DMI hint in the transaction. If found,
     * request a DMI region, ask the QEMU instance DMI manager for a DMI region
     * alias for it and map it on the CPU address space.
     *
     * Ideal, the whole operation could be done on the SystemC thread for
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
     // N.B.
     // we choose to protect all such areas with a mutex,
     // allowing us to process everything on the QEMU thread.
     * DEPRECATED ... effectively map the alias onto the root MR. This is why we first create
     * the alias on the SystemC thread and return it to the CPU thread. Once
     * we're back to the CPU thread, we lock the DMI manager again and check
     * for the alias validity flag. If an invalidation happened in between,
     * this flag will be false and we know we can throw the entire DMI request.
     *
     * If the alias is valid after we took the lock, we can map it. If an
     * invalidation must occur, it will be done after we release the lock.
     *
     * @see QemuInstanceDmiManager for more information on why we need a global
     * MR per DMI region.
     */
    void check_dmi_hint(TlmPayload& trans) {
        if (!trans.is_dmi_allowed()) {
            return;
        }

        SCP_INFO(SCMOD) << "DMI request for address 0x" << std::hex << trans.get_address();

        // It is 'safer' from the SystemC perspective to  m_on_sysc.run_on_sysc([this,
        // &trans]{...}).

        LockedQemuInstanceDmiManager dmi_mgr(m_inst.get_dmi_manager());
        if (!(*this)->get_direct_mem_ptr(trans, m_dmi_data)) {
            return;
        }

        // hold the lock through, as we will be doing several updates
        m_inst.get().lock_iothread();

        SCP_INFO(SCMOD) << "DMI Adding for address 0x" << std::hex << trans.get_address();

// this is relatively arbitary. The upper limit is set within QEMU by the TBU
// e.g. 4k small pages.
// Having 2000 separate chunks seems large enough.
#define MAX_MAP 2000
        // Prune if the map is to big before we start
        while (m_dmi_aliases.size() > MAX_MAP) {
            auto d = m_dmi_aliases.begin();
            std::advance(d, std::rand() % m_dmi_aliases.size());
            d = remove_alias(d);
        }

        uint64_t start = m_dmi_data.get_start_address();
        uint64_t end = m_dmi_data.get_end_address();

        if (m_dmi_aliases.size() > 0) {
            auto next = m_dmi_aliases.upper_bound(start);
            if (next != m_dmi_aliases.begin()) {
                auto prev = std::prev(next);
                DmiRegionAlias::Ptr dmi = prev->second;
                if (prev->first == start) {
                    // already have the DMI
                    assert(end <= dmi->get_end());
                    assert(m_dmi_data.get_dmi_ptr() == dmi->get_dmi_ptr());
                    m_inst.get().unlock_iothread();
                    SCP_INFO(SCMOD) << "Already have region";
                    return;
                }
                uint64_t sz = dmi->get_size();
                if (dmi->get_end() + 1 == start &&
                    dmi->get_dmi_ptr() + sz == m_dmi_data.get_dmi_ptr()) {
                    SCP_INFO(SCMOD) << "Merge with previous";
                    start = dmi->get_start();
                    m_dmi_data.set_start_address(start);
                    m_dmi_data.set_dmi_ptr(dmi->get_dmi_ptr());

                    remove_alias(prev);
                }
            }
            if (next != m_dmi_aliases.end()) {
                DmiRegionAlias::Ptr dmi = next->second;
                uint64_t sz = dmi->get_size();
                if (next->first == start) {
                    // already have the DMI
                    assert(end <= dmi->get_end());
                    assert(m_dmi_data.get_dmi_ptr() == dmi->get_dmi_ptr());
                    m_inst.get().unlock_iothread();
                    SCP_INFO(SCMOD) << "Already have region(2)";
                    return;
                }
                if (dmi->get_start() == end + 1 &&
                    m_dmi_data.get_dmi_ptr() + sz == dmi->get_dmi_ptr()) {
                    SCP_INFO(SCMOD) << "Merge with next";
                    end = dmi->get_end();
                    m_dmi_data.set_end_address(end);

                    remove_alias(next);
                }
            }
        }

        SCP_INFO(SCMOD) << "Adding DMI for range [0x" << std::hex << m_dmi_data.get_start_address()
                        << ", 0x" << std::hex << m_dmi_data.get_end_address() << "]";

        DmiRegionAlias::Ptr alias(dmi_mgr.get_new_region_alias(m_dmi_data));

        m_dmi_aliases[start] = alias;
        add_dmi_mr_alias(m_dmi_aliases[start]);

        m_inst.get().unlock_iothread();
    }

    void check_qemu_mr_hint(TlmPayload& trans) {
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
        m_r->m_root.add_subregion(mr, mapping_addr);
    }

    void do_regular_access(TlmPayload& trans) {
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

    void do_debug_access(TlmPayload& trans) {
        m_on_sysc.run_on_sysc([this, &trans] { (*this)->transport_dbg(trans); });
    }

    MemTxResult qemu_io_access(tlm::tlm_command command, uint64_t addr, uint64_t* val,
                               unsigned int size, MemTxAttrs attrs) {
        TlmPayload trans;
        if (m_finished)
            return qemu::MemoryRegionOps::MemTxError;

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

    MemTxResult qemu_io_read(uint64_t addr, uint64_t* val, unsigned int size, MemTxAttrs attrs) {
        return qemu_io_access(tlm::TLM_READ_COMMAND, addr, val, size, attrs);
    }

    MemTxResult qemu_io_write(uint64_t addr, uint64_t val, unsigned int size, MemTxAttrs attrs) {
        return qemu_io_access(tlm::TLM_WRITE_COMMAND, addr, &val, size, attrs);
    }

public:
    QemuInitiatorSocket(const char* name, QemuInitiatorIface& initiator, QemuInstance& inst)
        : TlmInitiatorSocket(name)
        , m_inst(inst)
        , m_on_sysc(sc_core::sc_gen_unique_name("initiator_run_on_sysc"))
        , m_initiator(initiator) {
        TlmInitiatorSocket::bind(*static_cast<tlm::tlm_bw_transport_if<>*>(this));
    }

    void init(qemu::Device& dev, const char* prop) {
        using namespace std::placeholders;

        qemu::LibQemu& inst = m_inst.get();
        qemu::MemoryRegionOpsPtr ops;

        m_r = new m_mem_obj(inst); // oot = inst.object_new<qemu::MemoryRegion>();
        ops = inst.memory_region_ops_new();

        ops->set_read_callback(std::bind(&QemuInitiatorSocket::qemu_io_read, this, _1, _2, _3, _4));
        ops->set_write_callback(
            std::bind(&QemuInitiatorSocket::qemu_io_write, this, _1, _2, _3, _4));
        ops->set_max_access_size(8);

        m_r->m_root.init_io(dev, TlmInitiatorSocket::name(), std::numeric_limits<uint64_t>::max(),
                            ops);

        dev.set_prop_link(prop, m_r->m_root);

        m_dev = dev;
    }

    void end_of_simulation() {
        m_finished = true;
        cancel_all();
    }

    // This could happen during void end_of_simulation() but there is a race with other units trying
    // to pull down their DMI's
    ~QemuInitiatorSocket() {
        //        dmimgr_lock();
        LockedQemuInstanceDmiManager dmi_mgr(m_inst.get_dmi_manager());
        cancel_all();
        delete m_r;
        m_r = nullptr;
        //        dmimgr_unlock();
    }
    void init_global(qemu::Device& dev) {
        using namespace std::placeholders;

        qemu::LibQemu& inst = m_inst.get();
        qemu::MemoryRegionOpsPtr ops;
        m_r = new m_mem_obj(inst); // oot = inst.object_new<qemu::MemoryRegion>();
        ops = inst.memory_region_ops_new();

        ops->set_read_callback(std::bind(&QemuInitiatorSocket::qemu_io_read, this, _1, _2, _3, _4));
        ops->set_write_callback(
            std::bind(&QemuInitiatorSocket::qemu_io_write, this, _1, _2, _3, _4));
        ops->set_max_access_size(8);

        m_r->m_root.init_io(dev, TlmInitiatorSocket::name(), std::numeric_limits<uint64_t>::max(),
                            ops);

        auto as = inst.address_space_get_system_memory();
        as->init(m_r->m_root, "global-peripheral-initiator", true);
        m_dev = dev;
    }

    void cancel_all() {
        m_on_sysc.cancel_all();
    }

    /* tlm::tlm_bw_transport_if<> */
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase, sc_core::sc_time& t) {
        /* Should not be reached */
        assert(false);
        return tlm::TLM_COMPLETED;
    }

    virtual AliasesIterator remove_alias(AliasesIterator it) {
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
        r->invalidate_region();

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

    virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range) {
        SCP_INFO(SCMOD) << "DMI invalidate [0x" << std::hex << start_range << ", 0x" << std::hex
                        << end_range << "]";

        LockedQemuInstanceDmiManager dmi_mgr(m_inst.get_dmi_manager());

        auto it = m_dmi_aliases.upper_bound(start_range);

        if (it != m_dmi_aliases.begin()) {
            /*
             * Start with the preceding region, as it may already cross the
             * range we must invalidate.
             */
            it--;
        }
        m_inst.get().lock_iothread();
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

            SCP_INFO(SCMOD) << "Invalidated region [0x" << std::hex << r->get_start() << ", 0x"
                            << std::hex << r->get_end() << "]";
        }
        m_inst.get().unlock_iothread();
    }
};

#endif
