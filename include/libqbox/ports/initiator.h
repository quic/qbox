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
    using DmiInfo = QemuInstanceDmiManager::DmiInfo;

protected:
    QemuInstance &m_inst;
    QemuInitiatorIface &m_initiator;
    qemu::Device m_dev;
    gs::RunOnSysC m_on_sysc;

    qemu::MemoryRegion m_root;

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

    /*
     * Create a memory region as an alias to the global memory region contained
     * in info. Map the newly created region onto our root MR.
     */
    void add_dmi_mr_alias(DmiInfo &info)
    {
        qemu::MemoryRegion alias = m_inst.get().object_new<qemu::MemoryRegion>();

        alias.init_alias(m_dev, "dmi-alias", info.mr, 0, info.get_size());
        m_root.add_subregion(alias, info.start);
    }

    /*
     * Check for the presence of the DMI hint in the transaction. If found,
     * request a DMI region, ask the QEMU instance DMI manager for a global MR
     * for it and create the corresponding alias.
     *
     * @see QemuInstanceDmiManager for more information on why we need a global
     * MR per DMI region.
     */
    void check_dmi_hint(TlmPayload &trans)
    {
        bool valid;

        tlm::tlm_dmi dmi_data;

        if (!trans.is_dmi_allowed()) {
            return;
        }

        m_on_sysc.run_on_sysc([this, &trans, &dmi_data, &valid] {
            valid = (*this)->get_direct_mem_ptr(trans, dmi_data);
        });

        if (!valid) {
            /* This is probably a bug in the target */
            return;
        }

        DmiInfo info(dmi_data);

        {
            LockedQemuInstanceDmiManager dmi_mgr(m_inst.get_dmi_manager());
            dmi_mgr.get_global_mr(info);
        }

        add_dmi_mr_alias(info);
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

