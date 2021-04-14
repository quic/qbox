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

#ifndef _LIBQBOX_PORTS_INITIATOR_H
#define _LIBQBOX_PORTS_INITIATOR_H

#include <functional>
#include <limits>
#include <cassert>

#include <tlm>

#include <libqemu-cxx/libqemu-cxx.h>

#include <greensocs/libgssync.h>

#include "libqbox/qemu-instance.h"
#include "libqbox/tlm-extensions/qemu-mr-hint.h"

class QemuInitiatorIface {
public:
    using TlmPayload = tlm::tlm_generic_payload;

    virtual void initiator_customize_tlm_payload(TlmPayload &payload) = 0;
    virtual sc_core::sc_time initiator_get_local_time() = 0;
    virtual void initiator_set_local_time(const sc_core::sc_time &) = 0;
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
class QemuInitiatorSocket : public tlm::tlm_initiator_socket<BUSWIDTH,
                                                             tlm::tlm_base_protocol_types,
                                                             1, sc_core::SC_ZERO_OR_MORE_BOUND>
                          , public tlm::tlm_bw_transport_if<> {
public:
    using TlmInitiatorSocket = tlm::tlm_initiator_socket<BUSWIDTH,
                                                         tlm::tlm_base_protocol_types,
                                                         1, sc_core::SC_ZERO_OR_MORE_BOUND>;
    using TlmPayload = tlm::tlm_generic_payload;
    using MemTxResult = qemu::MemoryRegionOps::MemTxResult;
    using MemTxAttrs = qemu::MemoryRegionOps::MemTxAttrs;
    using DmiRegion = QemuInstanceDmiManager::DmiRegion;
    using DmiRegionAlias = QemuInstanceDmiManager::DmiRegionAlias;

    using DmiRegionAliasKey = uint64_t;

protected:
    QemuInstance &m_inst;
    QemuInitiatorIface &m_initiator;
    qemu::Device m_dev;
    gs::RunOnSysC m_on_sysc;

    qemu::MemoryRegion m_root;

    std::map<DmiRegionAliasKey, DmiRegionAlias> m_dmi_aliases;

    void init_payload(TlmPayload &trans, tlm::tlm_command command, uint64_t addr,
                      uint64_t *val, unsigned int size)
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

    DmiRegionAliasKey get_dmi_region_alias_key(const tlm::tlm_dmi &info)
    {
        return info.get_start_address();
    }

    DmiRegionAliasKey get_dmi_region_alias_key(const DmiRegionAlias &alias)
    {
        return alias.get_start();
    }

    void add_dmi_mr_alias(DmiRegionAlias &alias)
    {
        qemu::MemoryRegion alias_mr = alias.get_alias_mr();

        GS_LOG("Adding DMI alias for region [%08" PRIx64 ", %08" PRIx64"]",
               alias.get_start(), alias.get_end());

        m_inst.get().lock_iothread();
        m_root.add_subregion(alias_mr, alias.get_start());
        m_inst.get().unlock_iothread();

        alias.set_installed();
    }

    void del_dmi_mr_alias(const DmiRegionAlias &alias)
    {
        qemu::MemoryRegion alias_mr = alias.get_alias_mr();

        if (!alias.is_installed()) {
            return;
        }

        GS_LOG("Removing DMI alias for region [%08" PRIx64 ", %08" PRIx64"]",
               alias.get_start(), alias.get_end());

        m_inst.get().lock_iothread();
        m_root.del_subregion(alias_mr);
        m_inst.get().unlock_iothread();
    }

    /*
     * Called on the SystemC thread. Returns a DMI region alias covering the
     * payload address.
     */
    DmiRegionAlias* request_dmi_region(TlmPayload &trans)
    {
        bool valid;
        tlm::tlm_dmi dmi_data;
        DmiRegionAliasKey key;

        valid = (*this)->get_direct_mem_ptr(trans, dmi_data);

        if (!valid) {
            /* This is probably a bug in the target. Return an invalid alias. */
            return nullptr;
        }

        GS_LOG("Got DMI for range [%08" PRIx64 ", %08" PRIx64 "]",
               dmi_data.get_start_address(),
               dmi_data.get_end_address());

        key = get_dmi_region_alias_key(dmi_data);

        assert(m_dmi_aliases.find(key) == m_dmi_aliases.end());

        LockedQemuInstanceDmiManager dmi_mgr(m_inst.get_dmi_manager());
        DmiRegionAlias alias(dmi_mgr.get_new_region_alias(dmi_data));

        m_dmi_aliases[key] = alias;
        return &m_dmi_aliases[key];
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
     * On the other hand we must do a bunch on the SystemC thread to ensure
     * validity of the DMI region and thus the alias until the point where we
     * effectively map the alias onto the root MR. This is why we first create
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
    void check_dmi_hint(TlmPayload &trans)
    {
        DmiRegionAliasKey key;
        DmiRegionAlias *alias;

        if (!trans.is_dmi_allowed()) {
            return;
        }

        GS_LOG("DMI request for address %08" PRIx64, trans.get_address());
        m_on_sysc.run_on_sysc([this, &trans, &alias] { alias = request_dmi_region(trans); });

        if (alias == nullptr) {
            return;
        }

        LockedQemuInstanceDmiManager dmi_mgr(m_inst.get_dmi_manager());

        key = get_dmi_region_alias_key(*alias);

        if (!alias->is_valid()) {
            /* This DMI region has been invalidated since we requested it */
            m_dmi_aliases.erase(key);
            return;
        }

        add_dmi_mr_alias(*alias);
    }

    void check_qemu_mr_hint(TlmPayload &trans)
    {
        QemuMrHintTlmExtension *ext = nullptr;
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

        qemu::MemoryRegion mr(m_inst.get().object_new<qemu::MemoryRegion>());

        mr.init_alias(m_dev, "mr-alias", target_mr, 0, target_mr.get_size());
        m_root.add_subregion(mr, mapping_addr);
    }

    void do_regular_access(TlmPayload &trans)
    {
        using sc_core::sc_time;

        uint64_t addr = trans.get_address();
        sc_time now = m_initiator.initiator_get_local_time();

        m_on_sysc.run_on_sysc([this, &trans, &now] {
            (*this)->b_transport(trans, now);
        });

        /*
         * Reset transaction address before dmi check (could be altered by
         * b_transport).
         */
        trans.set_address(addr);

        check_qemu_mr_hint(trans);
        check_dmi_hint(trans);

        m_initiator.initiator_set_local_time(now);
    }

    void do_debug_access(TlmPayload &trans)
    {
        m_on_sysc.run_on_sysc([this, &trans] {
            (*this)->transport_dbg(trans);
        });
    }

    MemTxResult qemu_io_access(tlm::tlm_command command,
                               uint64_t addr, uint64_t* val,
                               unsigned int size, MemTxAttrs attrs)
    {
        TlmPayload trans;

        m_inst.get().unlock_iothread();

        init_payload(trans, command, addr, val, size);

        if (attrs.debug) {
            do_debug_access(trans);
        } else {
            do_regular_access(trans);
        }

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

    MemTxResult qemu_io_read(uint64_t addr, uint64_t *val,
                             unsigned int size, MemTxAttrs attrs)
    {
        return qemu_io_access(tlm::TLM_READ_COMMAND, addr, val, size, attrs);
    }

    MemTxResult qemu_io_write(uint64_t addr, uint64_t val,
                              unsigned int size, MemTxAttrs attrs)
    {
        return qemu_io_access(tlm::TLM_WRITE_COMMAND, addr, &val, size, attrs);
    }

public:
    QemuInitiatorSocket(const char *name, QemuInitiatorIface &initiator, QemuInstance &inst)
        : TlmInitiatorSocket(name)
        , m_inst(inst)
        , m_on_sysc(sc_core::sc_gen_unique_name("initiator-run-on-sysc"))
        , m_initiator(initiator)
    {
        TlmInitiatorSocket::bind(*static_cast<tlm::tlm_bw_transport_if<>*>(this));
    }

    void init(qemu::Device &dev, const char *prop)
    {
        using namespace std::placeholders;

        qemu::LibQemu &inst = m_inst.get();
        qemu::MemoryRegionOpsPtr ops;

        m_root = inst.object_new<qemu::MemoryRegion>();
        ops = inst.memory_region_ops_new();

        ops->set_read_callback(std::bind(&QemuInitiatorSocket::qemu_io_read,
                                         this, _1, _2, _3, _4));
        ops->set_write_callback(std::bind(&QemuInitiatorSocket::qemu_io_write,
                                          this, _1, _2, _3, _4));
        ops->set_max_access_size(8);

        m_root.init_io(dev, TlmInitiatorSocket::name(),
                       std::numeric_limits<uint64_t>::max(), ops);

        dev.set_prop_link(prop, m_root);

        m_dev = dev;
    }
    /* tlm::tlm_bw_transport_if<> */
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& t)
    {
        /* Should not be reached */
        assert(false);
        return tlm::TLM_COMPLETED;
    }

    virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range,
                                           sc_dt::uint64 end_range)
    {
        GS_LOG("DMI invalidate: [%08" PRIx64 ", %08" PRIx64 "]",
               uint64_t(start_range), uint64_t(end_range));

        /* TODO */
    }
};

#endif

