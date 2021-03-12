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

class QemuToTlmInitiatorBridge : public tlm::tlm_bw_transport_if<> {
public:
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
        /* TODO */
    }
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
class QemuInitiatorSocket : public tlm::tlm_initiator_socket<BUSWIDTH> {
public:
    using TlmInitiatorSocket = tlm::tlm_initiator_socket<BUSWIDTH>;
    using TlmPayload = tlm::tlm_generic_payload;
    using MemTxResult = qemu::MemoryRegionOps::MemTxResult;
    using MemTxAttrs = qemu::MemoryRegionOps::MemTxAttrs;
    using DmiInfo = QemuInstanceDmiManager::DmiInfo;

protected:
    QemuToTlmInitiatorBridge m_bridge;
    QemuInstance &m_inst;
    qemu::Device m_dev;

    qemu::MemoryRegion m_root;

public:
    QemuInitiatorSocket(const char *name, QemuInstance &inst)
        : TlmInitiatorSocket(name)
        , m_inst(inst)
    {
        TlmInitiatorSocket::bind(m_bridge);
    }

    void init(qemu::Device &dev, const char *prop)
    {
        using namespace std::placeholders;

        qemu::LibQemu &inst = m_inst.get();
        qemu::MemoryRegionOpsPtr ops;

        m_root = inst.object_new<qemu::MemoryRegion>();
        ops = inst.memory_region_ops_new();

        ops->set_max_access_size(8);

        m_root.init_io(dev, TlmInitiatorSocket::name(),
                       std::numeric_limits<uint64_t>::max(), ops);

        dev.set_prop_link(prop, m_root);

        m_dev = dev;
    }
};

#endif

