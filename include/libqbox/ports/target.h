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

#ifndef _LIBQBOX_PORTS_TARGET_H
#define _LIBQBOX_PORTS_TARGET_H

#include <tlm>

class TlmTargetToQemuBridge : public tlm::tlm_fw_transport_if<> {
public:
    using MemTxAttrs = qemu::MemoryRegion::MemTxAttrs;
    using MemTxResult = qemu::MemoryRegion::MemTxResult;
    using TlmPayload = tlm::tlm_generic_payload;

protected:
    qemu::MemoryRegion m_mr;

public:
    void init(qemu::SysBusDevice sbd, int mmio_idx)
    {
        m_mr = sbd.mmio_get_region(mmio_idx);
    }

    virtual void b_transport(TlmPayload& trans,
                             sc_core::sc_time& t)
    {
        uint64_t addr = trans.get_address();
        uint64_t *data = reinterpret_cast<uint64_t *>(trans.get_data_ptr());
        unsigned int size = trans.get_data_length();
        MemTxAttrs attrs;
        MemTxResult res;

        if (trans.get_command() == tlm::TLM_IGNORE_COMMAND) {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
            return;
        }

        switch (trans.get_command()) {
        case tlm::TLM_READ_COMMAND:
            res = m_mr.dispatch_read(addr, data, size, attrs);
            break;

        case tlm::TLM_WRITE_COMMAND:
            res = m_mr.dispatch_write(addr, *data, size, attrs);
            break;

        default:
            /* TLM_IGNORE_COMMAND already handled above */
            assert(false);
            return;
        }

        switch (res) {
        case qemu::MemoryRegionOps::MemTxOK:
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
            break;

        case qemu::MemoryRegionOps::MemTxDecodeError:
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            break;

        case qemu::MemoryRegionOps::MemTxError:
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            break;
        }
    }

    virtual tlm::tlm_sync_enum nb_transport_fw(TlmPayload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& t)
    {
        /* TODO: report an error */
        abort();
        return tlm::TLM_ACCEPTED;
    }

    virtual bool get_direct_mem_ptr(TlmPayload& trans,
                                    tlm::tlm_dmi& dmi_data)
    {
        return false;
    }

    virtual unsigned int transport_dbg(TlmPayload& trans)
    {
        /* TODO: implement this as a standard QEMU transaction */
        return 0;
    }
};

template <unsigned int BUSWIDTH = 32>
class QemuTargetSocket : public tlm::tlm_target_socket<BUSWIDTH> {
public:
    using TlmTargetSocket = tlm::tlm_target_socket<BUSWIDTH>;
    using TlmPayload = tlm::tlm_generic_payload;

protected:
    TlmTargetToQemuBridge m_bridge;
    QemuInstance &m_inst;
    qemu::SysBusDevice m_sbd;

public:
    QemuTargetSocket(const char *name, QemuInstance &inst)
        : TlmTargetSocket(name)
        , m_inst(inst)
    {
        TlmTargetSocket::bind(m_bridge);
    }

    void init(qemu::SysBusDevice sbd, int mmio_idx)
    {
        m_bridge.init(sbd, mmio_idx);
    }
};

#endif
