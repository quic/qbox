/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include <systemc>

#include "backends/net-backend.h"
#include "components/dma.h"
#include "components/mac.h"

#include <deque>
#include <greensocs/gsutils/tlm_sockets_buswidth.h>

struct XGmacDesc {
    uint32_t ctl_stat;
    uint16_t buffer1_size;
    uint16_t buffer2_size;
    uint64_t buffer1_addr;
    uint64_t buffer2_addr;
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

class Xgmac : public sc_core::sc_module
{
public:
    sc_core::sc_out<bool> sbd_irq, pmt_irq, mci_irq;
    tlm_utils::simple_target_socket<Xgmac, DEFAULT_TLM_BUSWIDTH> socket;

    Dma dma;

protected:
    RxTxStats m_stats;
    uint32_t m_regs[R_MAX];

    bool eth_can_rx() const;
    ssize_t eth_rx(const uint8_t* buf, size_t size);
    void xgmac_read_desc(XGmacDesc* d, int rx);
    void xgmac_write_desc(XGmacDesc* d, int rx);
    void xgmac_enet_send();
    void enet_update_irq();
    void enet_update_irq_sysc();
    uint64_t enet_read(uint64_t addr, unsigned size);
    void enet_write(uint64_t addr, uint64_t value, unsigned size);

private:
    Payload m_tx_frame;

    MACAddress m_mac;

    sc_core::sc_event update_event;

public:
    SC_HAS_PROCESS(Xgmac);
    Xgmac(sc_core::sc_module_name name);
    virtual ~Xgmac();

    NetworkBackend* m_backend;
    void set_backend(NetworkBackend* backend)
    {
        m_backend = backend;
        m_backend->register_receive(this, eth_rx_sc, eth_can_rx_sc);
    }

    static void eth_rx_sc(void* opaque, Payload& frame) { ((Xgmac*)opaque)->eth_rx(frame.data(), frame.size()); }

    static int eth_can_rx_sc(void* opaque) { return ((Xgmac*)opaque)->eth_can_rx(); }

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        unsigned char* ptr = trans.get_data_ptr();
        uint64_t addr = trans.get_address();

        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            enet_write(addr, *(uint32_t*)ptr, 4);
            break;
        case tlm::TLM_READ_COMMAND:
            *(uint32_t*)ptr = enet_read(addr, 4);
            break;
        default:
            break;
        }
    }

    virtual void payload_recv(Payload& frame);
};
