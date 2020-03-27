/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps
 *  Copyright (c) 2020 QEMU
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

#pragma once

#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/simple_initiator_socket.h"

#include "libqbox/utils/mac.h"
#include "libqbox/backends/net-backend.h"

#include <deque>

struct XGmacDesc {
    uint32_t ctl_stat;
    uint16_t buffer1_size;
    uint16_t buffer2_size;
    uint32_t buffer1_addr;
    uint32_t buffer2_addr;
    uint32_t ext_stat;
    uint32_t res[3];
};

struct RxTxStats {
    uint64_t rx_bytes;
    uint64_t tx_bytes;

    uint64_t rx;
    uint64_t rx_bcast;
    uint64_t rx_mcast;
};

#define R_MAX 0x400

class Dma : public sc_core::sc_module {
public:
    tlm_utils::simple_initiator_socket<Dma> socket;

    Dma(const char *name)
        : sc_core::sc_module(name)
        , socket("socket")
    {
    }

    ~Dma()
    {
    }

    void bus_do(uint64_t addr, uint8_t *data, uint32_t size, bool is_write)
    {
        tlm::tlm_generic_payload trans;

        trans.set_command(is_write ? tlm::TLM_WRITE_COMMAND : tlm::TLM_READ_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(data));
        trans.set_data_length(size);
        trans.set_streaming_width(size);
        trans.set_byte_enable_length(0);
        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        sc_core::sc_time delay(sc_core::SC_ZERO_TIME);
        socket->b_transport(trans, delay);
    }

    void bus_write(uint64_t addr, uint8_t *data, uint32_t size)
    {
        bus_do(addr, data, size, 1);
    }

    void bus_read(uint64_t addr, uint8_t *data, uint32_t size)
    {
        bus_do(addr, data, size, 0);
    }
};

class Xgmac : public sc_core::sc_module {
protected:
    RxTxStats m_stats;
    uint32_t m_regs[R_MAX];

    bool eth_can_rx() const;
    ssize_t eth_rx(const uint8_t *buf, size_t size);
    void xgmac_read_desc(XGmacDesc *d, int rx);
    void xgmac_write_desc(XGmacDesc *d, int rx);
    void xgmac_enet_send();
    void enet_update_irq();
    uint64_t enet_read(uint64_t addr, unsigned size);
    void enet_write(uint64_t addr, uint64_t value, unsigned size);

public:
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> sbd_irq;
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> pmt_irq;
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> mci_irq;
    tlm_utils::simple_target_socket<Xgmac> socket;
    Dma dma;
    Payload m_tx_frame;

    MACAddress m_mac;

    SC_HAS_PROCESS(Xgmac);
    Xgmac(sc_core::sc_module_name name);
    virtual ~Xgmac();

    NetworkBackend *m_backend;
    void set_backend(NetworkBackend *backend)
    {
        m_backend = backend;
        m_backend->register_receive(this, eth_rx_sc, eth_can_rx_sc);
    }

    static void eth_rx_sc(void *opaque, Payload &frame)
    {
        ((Xgmac *)opaque)->eth_rx(frame.data(), frame.size());
    }

    static int eth_can_rx_sc(void *opaque)
    {
        return ((Xgmac *)opaque)->eth_can_rx();
    }

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        unsigned char *ptr = trans.get_data_ptr();
        uint64_t addr = trans.get_address();

        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            enet_write(addr, *(uint32_t *)ptr, 4);
            break;
        case tlm::TLM_READ_COMMAND:
            *(uint32_t *)ptr = enet_read(addr, 4);
            break;
        default:
            break;
        }
    }

    virtual void payload_recv(Payload &frame);
};
