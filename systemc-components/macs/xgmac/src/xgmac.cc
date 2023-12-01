/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arpa/inet.h>

#include <xgmac.h>

#include <scp/report.h>

using namespace sc_core;
using namespace std;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define XGMAC_CONTROL      0x00000000 /* MAC Configuration */
#define XGMAC_FRAME_FILTER 0x00000001 /* MAC Frame Filter */
#define XGMAC_FLOW_CTRL    0x00000006 /* MAC Flow Control */
#define XGMAC_VLAN_TAG     0x00000007 /* VLAN Tags */
#define XGMAC_VERSION      0x00000008 /* Version */
/* VLAN tag for insertion or replacement into tx frames */
#define XGMAC_VLAN_INCL  0x00000009
#define XGMAC_LPI_CTRL   0x0000000a /* LPI Control and Status */
#define XGMAC_LPI_TIMER  0x0000000b /* LPI Timers Control */
#define XGMAC_TX_PACE    0x0000000c /* Transmit Pace and Stretch */
#define XGMAC_VLAN_HASH  0x0000000d /* VLAN Hash Table */
#define XGMAC_DEBUG      0x0000000e /* Debug */
#define XGMAC_INT_STATUS 0x0000000f /* Interrupt and Control */
/* HASH table registers */
#define XGMAC_HASH(n)  ((0x00000300 / 4) + (n))
#define XGMAC_NUM_HASH 16
/* Operation Mode */
#define XGMAC_OPMODE (0x00000400 / 4)
/* Remote Wake-Up Frame Filter */
#define XGMAC_REMOTE_WAKE (0x00000700 / 4)
/* PMT Control and Status */
#define XGMAC_PMT (0x00000704 / 4)

#define XGMAC_ADDR_HIGH(reg) (0x00000010 + ((reg)*2))
#define XGMAC_ADDR_LOW(reg)  (0x00000011 + ((reg)*2))

#define DMA_BUS_MODE         0x000003c0 /* Bus Mode */
#define DMA_XMT_POLL_DEMAND  0x000003c1 /* Transmit Poll Demand */
#define DMA_RCV_POLL_DEMAND  0x000003c2 /* Received Poll Demand */
#define DMA_RCV_BASE_ADDR    0x000003c3 /* Receive List Base */
#define DMA_TX_BASE_ADDR     0x000003c4 /* Transmit List Base */
#define DMA_STATUS           0x000003c5 /* Status Register */
#define DMA_CONTROL          0x000003c6 /* Ctrl (Operational Mode) */
#define DMA_INTR_ENA         0x000003c7 /* Interrupt Enable */
#define DMA_MISSED_FRAME_CTR 0x000003c8 /* Missed Frame Counter */
/* Receive Interrupt Watchdog Timer */
#define DMA_RI_WATCHDOG_TIMER 0x000003c9
#define DMA_AXI_BUS           0x000003ca /* AXI Bus Mode */
#define DMA_AXI_STATUS        0x000003cb /* AXI Status */
#define DMA_CUR_TX_DESC_ADDR  0x000003d2 /* Current Host Tx Descriptor */
#define DMA_CUR_RX_DESC_ADDR  0x000003d3 /* Current Host Rx Descriptor */
#define DMA_CUR_TX_BUF_ADDR   0x000003d4 /* Current Host Tx Buffer */
#define DMA_CUR_RX_BUF_ADDR   0x000003d5 /* Current Host Rx Buffer */
#define DMA_HW_FEATURE        0x000003d6 /* Enabled Hardware Features */

/* DMA Status register defines */
#define DMA_STATUS_GMI         0x08000000 /* MMC interrupt */
#define DMA_STATUS_GLI         0x04000000 /* GMAC Line interface int */
#define DMA_STATUS_EB_MASK     0x00380000 /* Error Bits Mask */
#define DMA_STATUS_EB_TX_ABORT 0x00080000 /* Error Bits - TX Abort */
#define DMA_STATUS_EB_RX_ABORT 0x00100000 /* Error Bits - RX Abort */
#define DMA_STATUS_TS_MASK     0x00700000 /* Transmit Process State */
#define DMA_STATUS_TS_SHIFT    20
#define DMA_STATUS_RS_MASK     0x000e0000 /* Receive Process State */
#define DMA_STATUS_RS_SHIFT    17
#define DMA_STATUS_NIS         0x00010000 /* Normal Interrupt Summary */
#define DMA_STATUS_AIS         0x00008000 /* Abnormal Interrupt Summary */
#define DMA_STATUS_ERI         0x00004000 /* Early Receive Interrupt */
#define DMA_STATUS_FBI         0x00002000 /* Fatal Bus Error Interrupt */
#define DMA_STATUS_ETI         0x00000400 /* Early Transmit Interrupt */
#define DMA_STATUS_RWT         0x00000200 /* Receive Watchdog Timeout */
#define DMA_STATUS_RPS         0x00000100 /* Receive Process Stopped */
#define DMA_STATUS_RU          0x00000080 /* Receive Buffer Unavailable */
#define DMA_STATUS_RI          0x00000040 /* Receive Interrupt */
#define DMA_STATUS_UNF         0x00000020 /* Transmit Underflow */
#define DMA_STATUS_OVF         0x00000010 /* Receive Overflow */
#define DMA_STATUS_TJT         0x00000008 /* Transmit Jabber Timeout */
#define DMA_STATUS_TU          0x00000004 /* Transmit Buffer Unavailable */
#define DMA_STATUS_TPS         0x00000002 /* Transmit Process Stopped */
#define DMA_STATUS_TI          0x00000001 /* Transmit Interrupt */

/* DMA Control register defines */
#define DMA_CONTROL_ST  0x00002000 /* Start/Stop Transmission */
#define DMA_CONTROL_SR  0x00000002 /* Start/Stop Receive */
#define DMA_CONTROL_DFF 0x01000000 /* Disable flush of rx frames */

xgmac::xgmac(sc_module_name name)
    : sc_module(name)
    , sbd_irq("sbd_irq")
    , pmt_irq("pmt_irq")
    , mci_irq("mci_irq")
    , socket("socket")
    , m_dma("dma")
    , m_regs()
    , m_tx_frame(9018u)
{
    SCP_TRACE(SCMOD) << "Constructor";
    std::string mac = "0a:a0:db:c0:49:c3";
    if (!m_mac.set_from_str(mac)) {
        SCP_ERR(SCMOD) << "Invalid mac-address '" << mac << "'";
    }

    m_regs[XGMAC_ADDR_HIGH(0)] = m_mac.hi();
    m_regs[XGMAC_ADDR_LOW(0)] = m_mac.lo();

    socket.register_b_transport(this, &xgmac::b_transport);

    SC_METHOD(enet_update_irq_sysc);
    sensitive << update_event;
}

xgmac::~xgmac() {}

bool xgmac::eth_can_rx() const
{
    /* RX enabled?  */
    return m_regs[DMA_CONTROL] & DMA_CONTROL_SR;
}

ssize_t xgmac::eth_rx(const uint8_t* buf, size_t size)
{
    static const unsigned char sa_bcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    int unicast, broadcast, multicast;
    XGmacDesc bd;
    ssize_t ret;

    if (!eth_can_rx()) {
        return -1;
    }

    unicast = ~buf[0] & 0x1;
    broadcast = memcmp(buf, sa_bcast, 6) == 0;
    multicast = !unicast && !broadcast;
    if (size < 12) {
        m_regs[DMA_STATUS] |= DMA_STATUS_RI | DMA_STATUS_NIS;
        ret = -1;
        goto out;
    }

    xgmac_read_desc(&bd, 1);
    if ((bd.ctl_stat & 0x80000000) == 0) {
        m_regs[DMA_STATUS] |= DMA_STATUS_RU | DMA_STATUS_AIS;
        ret = size;
        goto out;
    }

    m_dma.bus_write(((bd.buffer2_addr << 32) | bd.buffer1_addr), (uint8_t*)buf, size);

    /* Add in the 4 bytes for crc (the real hw returns length incl crc) */
    size += 4;
    bd.ctl_stat = (size << 16) | 0x300;
    xgmac_write_desc(&bd, 1);

    m_stats.rx_bytes += size;
    m_stats.rx++;
    if (multicast) {
        m_stats.rx_mcast++;
    } else if (broadcast) {
        m_stats.rx_bcast++;
    }

    m_regs[DMA_STATUS] |= DMA_STATUS_RI | DMA_STATUS_NIS;
    ret = size;

out:
    enet_update_irq();
    return ret;
}

void xgmac::payload_recv(Payload& frame) { eth_rx(frame.data(), frame.size()); }

void xgmac::xgmac_read_desc(XGmacDesc* d, int rx)
{
    uint32_t addr = rx ? m_regs[DMA_CUR_RX_DESC_ADDR] : m_regs[DMA_CUR_TX_DESC_ADDR];

    SCP_TRACE(SCMOD) << "Reading XGmacDesc at addr " << std::hex << addr;
    m_dma.bus_read(addr, (uint8_t*)d, sizeof(*d));
}

void xgmac::xgmac_write_desc(XGmacDesc* d, int rx)
{
    int reg = rx ? DMA_CUR_RX_DESC_ADDR : DMA_CUR_TX_DESC_ADDR;
    uint32_t addr = m_regs[reg];

    if (!rx && (d->ctl_stat & 0x00200000)) {
        m_regs[reg] = m_regs[DMA_TX_BASE_ADDR];
    } else if (rx && (d->buffer1_size & 0x8000)) {
        m_regs[reg] = m_regs[DMA_RCV_BASE_ADDR];
    } else {
        m_regs[reg] += sizeof(*d);
    }

    m_dma.bus_write(addr, (uint8_t*)d, sizeof(*d));
}

void xgmac::xgmac_enet_send()
{
    XGmacDesc bd;
    int frame_size;
    int len;
    uint8_t frame[8192];
    uint8_t* ptr;

    ptr = frame;
    frame_size = 0;
    while (1) {
        xgmac_read_desc(&bd, 0);
        if ((bd.ctl_stat & 0x80000000) == 0) {
            /* Run out of descriptors to transmit.  */
            break;
        }
        len = (bd.buffer1_size & 0xfff) + (bd.buffer2_size & 0xfff);

        if ((bd.buffer1_size & 0xfff) > 2048) {
            SCP_ERR(SCMOD) << __func__ << ":ERROR...ERROR...ERROR... -- xgmac buffer 1 len on send > 2048 (0x"
                           << std::hex << bd.buffer1_size << ")";
        }
        if ((bd.buffer2_size & 0xfff) != 0) {
            SCP_ERR(SCMOD) << __func__ << ":ERROR...ERROR...ERROR... -- xgmac buffer 2 len on send > 2048 (0x"
                           << std::hex << bd.buffer2_size << ")";
        }
        if (size_t(len) >= sizeof(frame)) {
            SCP_ERR(SCMOD) << __func__ << ": buffer overflow " << len << " read into " << sizeof(frame);
            SCP_ERR(SCMOD) << __func__ << ": buffer1.size=" << bd.buffer1_size << "; buffer2.size=" << bd.buffer2_size;
        }

        m_dma.bus_read(((bd.buffer2_addr << 32) | bd.buffer1_addr), ptr, len);
        ptr += len;
        frame_size += len;
        if (bd.ctl_stat & 0x20000000) {
            /* Last buffer in frame.  */
            SCP_TRACE(SCMOD) << "Last buffer in frame, sending. Size: " << frame_size;
            m_tx_frame.resize(frame_size);
            memcpy(m_tx_frame.data(), frame, frame_size);
            m_backend->send(m_tx_frame);

            ptr = frame;
            frame_size = 0;
            m_regs[DMA_STATUS] |= DMA_STATUS_TI | DMA_STATUS_NIS;
        }
        bd.ctl_stat &= ~0x80000000;

        /* Write back the modified descriptor.  */
        xgmac_write_desc(&bd, 0);
    }
}

void xgmac::enet_update_irq() { update_event.notify(); }

void xgmac::enet_update_irq_sysc()
{
    int stat = m_regs[DMA_STATUS] & m_regs[DMA_INTR_ENA];
    sbd_irq = !!stat;
}

uint64_t xgmac::enet_read(uint64_t addr, unsigned size)
{
    uint64_t r = 0;
    addr >>= 2;

    switch (addr) {
    case XGMAC_VERSION:
        r = 0x1012;
        break;
    default:
        if (addr < ARRAY_SIZE(m_regs)) {
            r = m_regs[addr];
        }
        break;
    }
    return r;
}

void xgmac::enet_write(uint64_t addr, uint64_t value, unsigned size)
{
    addr >>= 2;
    switch (addr) {
    case DMA_BUS_MODE:
        m_regs[DMA_BUS_MODE] = value & ~0x1;
        break;
    case DMA_XMT_POLL_DEMAND:
        xgmac_enet_send();
        break;
    case DMA_STATUS:
        m_regs[DMA_STATUS] = m_regs[DMA_STATUS] & ~value;
        break;
    case DMA_RCV_BASE_ADDR:
        m_regs[DMA_RCV_BASE_ADDR] = m_regs[DMA_CUR_RX_DESC_ADDR] = value;
        break;
    case DMA_TX_BASE_ADDR:
        m_regs[DMA_TX_BASE_ADDR] = m_regs[DMA_CUR_TX_DESC_ADDR] = value;
        break;
    default:
        if (addr < ARRAY_SIZE(m_regs)) {
            m_regs[addr] = value;
        }
        break;
    }
    enet_update_irq();
}

void module_register()
{
    GSC_MODULE_REGISTER_C(xgmac);
}
