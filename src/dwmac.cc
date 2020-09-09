/*
 *  This file is part of Rabbits
 *  Copyright (C) 2015  Clement Deschamps and Luc Michel
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

#include <arpa/inet.h>

#include "utils/mac.h"
#include "models/dwmac.h"

#if 0
#define MLOG(foo, bar) \
    std::cout << "[" << "] "
#define MLOG_F(foo, bar, args...)\
    printf(args)
#else
#include <fstream>
#define MLOG(foo, bar) \
    if (0) std::cout
#define MLOG_F(foo, bar, args...)
#endif

using namespace sc_core;
using namespace std;

Dwmac::Dwmac(sc_module_name name)
    : sc_module(name)
    , intr0("mac-irq")
    , intr1("eth-wake-irq")
    , socket("socket")
    , dma("dma")
    , phy("phy", 0x00010001, 0)
    , m_tx_frame(9018u)
{
    reset();

    std::string mac = "0a:a0:db:c0:49:c3";
    MLOG(APP, TRC) << mac << "\n";
    if (!m_mac_0.set_from_str(mac)) {
        MLOG_F(APP, ERR, "Invalid mac-address '%s'\n", mac.c_str());
    }
    MLOG_F(APP, TRC, "Mac-address initialized to %02x:%02x:%02x:%02x:%02x:%02x\n",
            m_mac_0[0], m_mac_0[1], m_mac_0[2],
            m_mac_0[3], m_mac_0[4], m_mac_0[5]);

    for(int i=0; i<8; i++) {
        m_htbl[i] = 0;
    }

    m_int_mask = 0;

    socket.register_b_transport(this, &Dwmac::b_transport);

    SC_THREAD(tx_thread);
    SC_THREAD(intr_thread);
}

Dwmac::~Dwmac() { }

uint32_t Dwmac::read_word(uint32_t address)
{
    if ((address & 0x03)) {
        MLOG_F(SIM, DBG, "requested address not aligned\n");
        return 0;
    }

    if (address >= DWMAC_MACn && address <  DWMAC_MACn + (MACn_COUNT * 8)) {
        // Programmation of an extra MAC address
        bool islow    = ((address - DWMAC_MACn) >> 2) & 0x01;
        std::size_t macindex = ((address - DWMAC_MACn) >> 3);

        assert(macindex < MACn_COUNT);
        if (islow)
            return m_mac_n[macindex].lo();
        else
            return m_mac_n[macindex].hi();
    }

    switch(address){
    case DMA_BUS_MODE :
        return m_dma.bus_mode.value;
    case DMA_TX_POLL:
        return 0;
    case DMA_RX_POLL:
        return 0;
    case DMA_RX_DESC_LIST_ADDR:
        return m_dma.rx_desc_addr;
    case DMA_TX_DESC_LIST_ADDR:
        return m_dma.tx_desc_addr;
    case DMA_STATUS:
        return m_dma.intr.status();
    case DMA_OPERATION_MODE:
        return m_dma.opr_mode.value;
    case DMA_INTERRUPTS:
        return m_dma.intr.interrupts();
    case DMA_COUNTERS:
        return m_dma.counters;
    case DMA_CURRENT_TX_DESC:
        return m_dma.current_tx_desc;
    case DMA_CURRENT_RX_DESC:
        return m_dma.current_rx_desc;
    case DMA_CURRENT_TX_BUF:
        return m_dma.current_tx_buf;
    case DMA_CURRENT_RX_BUF:
        return m_dma.current_rx_buf;

    case DWMAC_CONFIGURATION:
        return m_configuration;
    case DWMAC_FRAME_FILTER:
        return m_frame_filter;
    case DWMAC_HASH_TABLE_HI:
        return m_hash_table_hi;
    case DWMAC_HASH_TABLE_LO:
        return m_hash_table_lo;

    case DWMAC_GMII_ADDRESS:
        return m_gmii_address.value;

    case DWMAC_GMII_DATA:
        {
            uint16_t data = 0;
            bool err = true;
#if 0
            phy.mdio_reg_read(m_gmii_address.pa, m_gmii_address.value, data, err);
#else
            if (m_gmii_address.pa == 0) {
                phy.mdio_reg_read(m_gmii_address.gr, data);
                err = false;
            }
#endif
            if (err) {
                MLOG_F(SIM, DBG, "read: gmii PHY %d not present\n", m_gmii_address.pa);
                data = 0xffff;
            }
            return data;
        }

    case DWMAC_FLOW_CONTROL:
        return m_flow_control;
    case DWMAC_VLAN_TAG:
        return m_vlan_tag;

    case DWMAC_VERSION:
        return DWMAC_VERSION_VALUE;

    case DWMAC_WAKE_UP_FILTER:
        MLOG_F(SIM, ERR, "WakeUp register no supported\n");
        abort();

    case DWMAC_PMT:
        return m_pmt;

    case DWMAC_INTERRUPT:
        return 0;

    case DWMAC_INTERRUPT_MASK:
        return m_int_mask;

    case DWMAC_MAC0_HI:
        return m_mac_0.hi();
    case DWMAC_MAC0_LO:
        return m_mac_0.lo();

    case DWMAC_AN_CONTROL:
    case DWMAC_AN_STATUS:
    case DWMAC_AN_ADVERTISEMENT:
    case DWMAC_AN_LINK_PARTNER_ABILITY:
    case DWMAC_AN_EXPANSION:
        if (warn_anregs) {
            MLOG_F(SIM, WRN, "read to A-N registers not supported (0x%x))(will warn only once)\n", (unsigned) address);
            warn_anregs = false;
        } else {
            MLOG_F(SIM, DBG, "read to A-N registers not supported (0x%x))\n", (unsigned) address);
        }
        return 0;

    case DWMAC_MMC_CONTROL:
    case DWMAC_MMC_RX_INT:
    case DWMAC_MMC_TX_INT:
    case DWMAC_MMC_RX_INT_MASK:
    case DWMAC_MMC_TX_INT_MASK:
    case DWMAC_MMC_IPC_RX_INT_MASK:
    case DWMAC_MMC_IPC_RX_INT:
        /* counters not implemented */
        return 0;

    case 0x500 ... 0x51C:
        return m_htbl[(address - 0x500) / 4];

    case DMA_RX_INT_WDT:
        /* receive interrupt watchdog timer not implemented */
        return 0;

    case DMA_AXI_BUS_MODE:
        return m_axi_bus_mode;

    case HW_FEATURE:
        {
            uint32_t hw_feature = 0x10D7F37; // Default value (from CycloneV datasheet)
            hw_feature &= ~(1 << 14); // Energy Efficient Ethernet disabled (because it triggers not yet supported MMD read)
            hw_feature |= (1 << 24); // Enhanced Descriptor Select enabled
            return hw_feature;
        }

    default:
        MLOG_F(SIM, DBG, "invalid read at address 0x%" PRIx32 "\n", address);
        return 0x00000000;
    }
}

void Dwmac::write_word(uint32_t address, uint32_t data)
{
    dma_desc desc;

    if ((address & 0x03)) {
        MLOG_F(SIM, DBG, "requested address not aligned\n");
        return ;
    }

    if (address >= DWMAC_MACn && address <  DWMAC_MACn + (MACn_COUNT * 8)) {
        bool islow    = ((address - DWMAC_MACn) >> 2) & 0x01;
        std::size_t macindex = ((address - DWMAC_MACn) >> 3);

        assert(macindex < MACn_COUNT);
        if (islow)
            m_mac_n[macindex].set_lo(data);
        else
            m_mac_n[macindex].set_hi(data);

        return ;
    }

    switch(address){
    case DMA_BUS_MODE :
        m_dma.bus_mode.value = data;
        MLOG_F(SIM, DBG, "Dma_BusMode: dsl=%d, atds=%d\n", m_dma.bus_mode.dsl, m_dma.bus_mode.atds);
        if (m_dma.bus_mode.swr)
            reset();
        return;

    case DMA_TX_POLL:
        read_tx_desc(&desc);
        if(!desc.des01.etx.own) {
            /* owned by host : suspend TX */
            m_dma.intr.m_dma_reg_status.tu = 1;
            m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
            m_tx_event.notify();
        }
        else {
            /* resume TX */
            m_dma.intr.setTxState(DwmacState::TX_SUSPENDED); // TODO: shoud be active state (but it is not yet modelized)
            m_tx_event.notify();
        }
        return;

    case DMA_RX_POLL:
        read_rx_desc(&desc);
        if(!desc.des01.erx.own) {
            /* owned by host : suspend RX */
            m_dma.intr.m_dma_reg_status.ru = 0;
            m_dma.intr.setRxState(DwmacState::RX_SUSPENDED);
        }
        else {
            /* resume RX */
            m_dma.intr.setRxState(DwmacState::RX_SUSPENDED); // TODO: shoud be active state (but it is not yet modelized)
            // TODO: should wake up the 'wait(delay)'
        }
        return;

    case DMA_RX_DESC_LIST_ADDR:
        if (m_dma.intr.getRxState() != DwmacState::RX_STOPPED) {
            MLOG_F(SIM, DBG, "invalid write of RxDescrListAddr when Rx active\n");
        }
        else {
            m_dma.rx_desc_addr = data;
            m_dma.current_rx_desc = m_dma.rx_desc_addr;
        }
        return;

    case DMA_TX_DESC_LIST_ADDR:
        if (m_dma.intr.getTxState() != DwmacState::TX_STOPPED) {
            MLOG_F(SIM, DBG, "invalid write of TxDescrListAddr when Tx active\n");
        }
        else {
            m_dma.tx_desc_addr = data;
            m_dma.current_tx_desc = m_dma.tx_desc_addr;
        }
        return;

    case DMA_STATUS:
        m_dma.intr.set_status(data);
        intr_wake.notify();
        return;

    case DMA_OPERATION_MODE:
        m_dma.opr_mode.value = data;
        m_dma.opr_mode.ftf = 0;
        if (m_dma.opr_mode.sr) {
            if (m_dma.intr.getRxState() == DwmacState::RX_STOPPED)
                m_dma.intr.setRxState(DwmacState::RX_SUSPENDED);
        }
        if (m_dma.opr_mode.st) {
            if (m_dma.intr.getTxState() == DwmacState::TX_STOPPED) {
                m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
                m_tx_event.notify();
            }
        }
        return;

    case DMA_INTERRUPTS:
        m_dma.intr.set_interrupts(data);
        intr_wake.notify();
        return;

    case DMA_COUNTERS:
        m_dma.counters = data;
        return;

    case DMA_CURRENT_TX_DESC:
        MLOG_F(SIM, ERR, "cannot write DMA_CURRENT_TX_DESC\n");
        return ;

    case DMA_CURRENT_RX_DESC:
        MLOG_F(SIM, ERR, "cannot write DMA_CURRENT_RX_DESC\n");
        return ;

    case DMA_CURRENT_TX_BUF:
        MLOG_F(SIM, ERR, "cannot write DMA_CURRENT_TX_BUF\n");
        return ;

    case DMA_CURRENT_RX_BUF:
        MLOG_F(SIM, ERR, "cannot write DMA_CURRENT_RX_BUF\n");
        return ;

    case DWMAC_CONFIGURATION:
        m_configuration = data;
        return ;

    case DWMAC_FRAME_FILTER:
        m_frame_filter = data;
        return ;

    case DWMAC_HASH_TABLE_HI:
        m_hash_table_hi = data;
        return ;

    case DWMAC_HASH_TABLE_LO:
        m_hash_table_lo = data;
        return ;

    case DWMAC_GMII_ADDRESS:
        m_gmii_address.value = data;
        m_gmii_address.gb = 0;
        return ;

    case DWMAC_GMII_DATA:
        {
            bool err = true;
#if 0
            phy.mdio_reg_write(m_gmii_address.pa, m_gmii_address.value, data, err);
#else
            if (m_gmii_address.pa == 0) {
                phy.mdio_reg_write(m_gmii_address.gr, data);
                err = false;
            }
#endif
            if (err) {
                MLOG_F(SIM, DBG, "write: gmii PHY %d not present\n", m_gmii_address.pa);
            }
            return;
        }

    case DWMAC_FLOW_CONTROL:
        m_flow_control = data;
        return;

    case DWMAC_VLAN_TAG:
        m_vlan_tag = data;
        return;

    case DWMAC_VERSION:
        MLOG_F(SIM, DBG, "version register is read-only\n");
        return ;

    case DWMAC_WAKE_UP_FILTER:
        MLOG_F(SIM, ERR, "WakeUp register not supported\n");
        abort();

    case DWMAC_PMT:
        m_pmt = data;
        return;

    case DWMAC_INTERRUPT:
        MLOG_F(SIM, DBG, "interrupt status register is read-only\n");
        return;

    case DWMAC_INTERRUPT_MASK:
        m_int_mask = data;
        return;

    case DWMAC_MAC0_HI:
        m_mac_0.set_hi(data);
        return;

    case DWMAC_MAC0_LO:
        m_mac_0.set_lo(data);
        return;

    case DWMAC_AN_CONTROL:
    case DWMAC_AN_STATUS:
    case DWMAC_AN_ADVERTISEMENT:
    case DWMAC_AN_LINK_PARTNER_ABILITY:
    case DWMAC_AN_EXPANSION:
        MLOG_F(SIM, ERR, "write to A-N registers not supported (0x%x))\n", (unsigned) address);
        return;

    case DWMAC_MMC_CONTROL:
    case DWMAC_MMC_RX_INT_MASK:
    case DWMAC_MMC_TX_INT_MASK:
    case DWMAC_MMC_IPC_RX_INT_MASK:
        /* counters not implemented */
        break;

    case 0x500 ... 0x51C:
        m_htbl[(address - 0x500) / 4] = data;
        // TODO: filter incoming frames according to the hash table value
        break;

    case DMA_RX_INT_WDT:
        /* receive interrupt watchdog timer not implemented */
        break;

    case DMA_AXI_BUS_MODE:
        m_axi_bus_mode = data;
        break;

    default:
        MLOG_F(SIM, DBG, "invalid write at address 0x%" PRIx32 "\n", address);
        return;
    }
}

void Dwmac::interrupts_fire(uint32_t interrupts) {
    m_dma.intr.fire(interrupts);
    intr_wake.notify();
}

void Dwmac::reset() {
    MLOG(SIM, TRC) << "Reset\n";

    m_dma.bus_mode.value    = 0x00000000;
    m_dma.rx_desc_addr      = 0x00000000;
    m_dma.tx_desc_addr      = 0x00000000;
    m_dma.intr.reset();
    m_dma.opr_mode.value    = 0x00000000;
    m_dma.counters          = 0x00000000;
    m_dma.current_tx_desc   = 0x00000000;
    m_dma.current_rx_desc   = 0x00000000;
    m_dma.current_tx_buf    = 0x00000000;
    m_dma.current_rx_buf    = 0x00000000;
    m_configuration			= 0x00000000;
    m_frame_filter   		= 0x00000000;
    m_hash_table_lo     	= 0x00000000;
    m_hash_table_hi     	= 0x00000000;
    m_gmii_address.value  	= 0x00000000;
    m_gmii_data     		= 0x00000000;
    m_flow_control   		= 0x00000000;
    m_vlan_tag          	= 0x00000000;
    m_pmt       			= 0x00000000;
    m_axi_bus_mode          = 0x00110001;
    m_mac_0.zero();

    for (uint32_t i = 0; i < MACn_COUNT; ++i)
        m_mac_n[i].zero();
}

bool Dwmac::read_tx_desc(dma_desc *desc) {
    dma.bus_read(m_dma.current_tx_desc, (uint8_t*)desc, m_dma.desc_size());

    if (!desc->des01.etx.own)
        return false;

    return true;
}

bool Dwmac::read_rx_desc(dma_desc *desc) {
    dma.bus_read(m_dma.current_rx_desc, (uint8_t*)desc, m_dma.desc_size());

    // clear status ?
    // desc->des01.rx &= 0x80000000;

    if (!desc->des01.erx.own)
        return false;

    return true;
}

void Dwmac::move_to_next_tx_desc(dma_desc &desc) {
    MLOG_F(SIM, TRC, "moving to the next TX descriptor\n");

    if(desc.des01.etx.end_ring) {
        MLOG_F(SIM, TRC, "TX: ring : end reached, pointing back to the first entry\n");
        m_dma.current_tx_desc = m_dma.tx_desc_addr;
    }
    else {
        if(desc.des01.etx.second_address_chained) {
            m_dma.current_tx_desc = desc.des3;
        }
        else {
            if(desc.des01.etx.buffer2_size > 0 || desc.des3 != 0) {
                MLOG_F(SIM, ERR, "TX: second buffer not supported !\n");
                exit(1);
            }
            m_dma.current_tx_desc += m_dma.desc_size() + (m_dma.bus_mode.dsl * 4);
        }
    }
}

static uint16_t checksum(const uint8_t *buf, int size, uint32_t sum)
{
    int i;

    /* Accumulate checksum */
    for (i = 0; i < size - 1; i += 2) {
        sum += *(uint16_t *) &buf[i];
    }

    /* Handle odd-sized case */
    if (size & 1) {
        sum += buf[i];
    }

    /* Fold to get the ones-complement result */
    while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

    /* Invert to get the negative in ones-complement arithmetic */
    return ~sum;
}

bool Dwmac::tx() {
    dma_desc desc;

    if (m_dma.intr.getTxState() != DwmacState::TX_SUSPENDED) {
        MLOG_F(SIM, DBG, "entering transmit routine without being in SUSP mode\n");
        return false;
    }

    m_dma.intr.setTxState(DwmacState::TX_FETCHING);

    if (!read_tx_desc(&desc)) {
        m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
        interrupts_fire(InterruptRegister::TxBufUnavailable);
        return false;
    }

    if (!desc.des01.etx.first_segment) {
        MLOG_F(SIM, ERR, "descriptor is not marked as being the first segment\n");
        m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
        return false;
    }

    m_dma.intr.setTxState(DwmacState::TX_READING);

    m_tx_frame.resize(desc.des01.etx.buffer1_size);
    dma.bus_read(desc.des2, m_tx_frame.data(), desc.des01.etx.buffer1_size);

    // fetching the whole frame
    while(!desc.des01.etx.last_segment) {
        uint32_t cur_desc_addr = m_dma.current_tx_desc;

        move_to_next_tx_desc(desc);

        // disown descriptor
        desc.des01.etx.own = 0;
        dma.bus_write(cur_desc_addr, (uint8_t*)&desc, m_dma.desc_size());

        // read next descriptor
        if(!read_tx_desc(&desc)) {
            MLOG_F(SIM, ERR, "cannot read descriptor\n");
            exit(1);
        }
        int offset = m_tx_frame.size();
        m_tx_frame.resize(offset + desc.des01.etx.buffer1_size);
        dma.bus_read(desc.des2, m_tx_frame.data() + offset, desc.des01.etx.buffer1_size);
    }

    if (m_tx_frame.size() < 14) {
        MLOG_F(SIM, DBG, "ethernet buffer size too small\n");
        m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
        return false;
    }

    /* Checksum offloading */
    if((m_configuration & (1 << 10)) && desc.des01.etx.checksum_insertion) {
        uint8_t *buffer = m_tx_frame.data();
        uint32_t len = m_tx_frame.size();

        if(len >= 14 && buffer[12] == 0x08 && buffer[13] == 0x00) {
            // IPv4 packet
            uint8_t *ipv4_pkt = &buffer[14];
            uint32_t ipv4_header_len = 4 * (ipv4_pkt[0] & 0x0f);

            /* IPv4 checksum */
            if(len >= (14 + ipv4_header_len)) {
                *(uint16_t *)(ipv4_pkt + 10) = 0;
                uint32_t csum = checksum(ipv4_pkt, ipv4_header_len, 0);
                //printf("Calculated checksum: %02x%02x\n", (csum >> 8) & 0xff, csum & 0xff);
                *(uint16_t *)(ipv4_pkt + 10) = csum;
            }

            const uint8_t ip_prot = ipv4_pkt[9];

            /* TCP/UDP checksum */
            if((desc.des01.etx.checksum_insertion == 3) && len >= 24 && (ip_prot == 0x06 || ip_prot == 0x11)) {
                int csum_offset;

                switch (ip_prot) {
                case 0x06:
                    /* TCP */
                    csum_offset = 16;
                    break;

                case 0x11:
                    /* UDP */
                    csum_offset = 6;
                    break;

                default:
                    assert(false);
                }

                uint8_t *tcp_seg = &ipv4_pkt[ipv4_header_len];
                //printf("Old checksum: %02X%02x\n", tcp_seg[16], tcp_seg[17]);

                /* Erase old checksum */
                *(uint16_t *)(tcp_seg + csum_offset) = 0x0000;

                uint32_t csum = 0;

                /* Pseudo header */
                csum += *(uint16_t *)(ipv4_pkt + 12);
                csum += *(uint16_t *)(ipv4_pkt + 14);
                csum += *(uint16_t *)(ipv4_pkt + 16);
                csum += *(uint16_t *)(ipv4_pkt + 18);
                csum += htons(ip_prot);
                csum += htons(len - 14 - ipv4_header_len);

                /* TCP/UDP segment */
                int size = len - 14 - ipv4_header_len;
                csum = checksum(tcp_seg, size, csum);
                //printf("Calculated checksum: %02x%02x\n", (csum >> 8) & 0xff, csum & 0xff);

                *(uint16_t *)(tcp_seg + csum_offset) = csum;
            }
        }
    }

    MLOG_F(SIM, TRC, "sending frame of length %u\n", (unsigned) m_tx_frame.size());
    m_backend->send(m_tx_frame);

    m_dma.intr.setTxState(DwmacState::TX_CLOSING);

    // disown descriptor
    desc.des01.etx.own = 0;
    dma.bus_write(m_dma.current_tx_desc, (uint8_t*)&desc, m_dma.desc_size());

    move_to_next_tx_desc(desc);

    if (desc.des01.etx.interrupt)
        interrupts_fire(InterruptRegister::TxInt);

    m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
    return true;
}

bool Dwmac::rx(Payload &frame) {
    if (frame.size() < 14)
        return false;

    dma_desc desc;

    if(!m_dma.opr_mode.sr) {
        MLOG_F(SIM, DBG, "DMA not started\n");
        return false;
    }

    if (m_dma.intr.getRxState() != DwmacState::RX_SUSPENDED) {
        MLOG_F(SIM, DBG, "entering receive routine without being in SUSP mode\n");
        return false;
    }

    m_dma.intr.setRxState(DwmacState::RX_FETCHING);

    if (!read_rx_desc(&desc)) {
        m_dma.intr.setRxState(DwmacState::RX_SUSPENDED);
        interrupts_fire(InterruptRegister::RxBufUnavailable);
        return false;
    }

    m_dma.intr.setRxState(DwmacState::RX_TRANSFERRING);

    if (desc.des01.erx.buffer1_size < frame.size()) {
        MLOG_F(SIM, DBG, "RX buffer too small\n");
        return false;
    }

    dma.bus_write(desc.des2, frame.data(), (int)frame.size());

    m_dma.intr.setRxState(DwmacState::RX_CLOSING);
    desc.des01.erx.first_descriptor = 1;
    desc.des01.erx.last_descriptor = 1;
    desc.des01.erx.frame_length = frame.size() + 4;

    desc.des01.erx.frame_type = 1; // if LT > 0x0600 (for example 0x0806 for IP)
    desc.des01.erx.rx_mac_addr = 0; // extended status not available (we would need to implement it)

    desc.des01.erx.own = 0;
    dma.bus_write(m_dma.current_rx_desc, (uint8_t*)&desc, m_dma.desc_size());

    MLOG_F(SIM, TRC, "moving to the next RX descriptor\n");

    if(desc.des01.erx.end_ring) {
        MLOG_F(SIM, TRC, "RX: ring : end reached, pointing back to the first entry\n");
        m_dma.current_rx_desc = m_dma.rx_desc_addr;
    }
    else {
        if(desc.des01.erx.second_address_chained) {
            m_dma.current_rx_desc = desc.des3;
        }
        else {
            m_dma.current_rx_desc += m_dma.desc_size() + (m_dma.bus_mode.dsl * 4);
        }
    }

    if (!desc.des01.erx.disable_ic) {
        interrupts_fire(InterruptRegister::RxInt);
    }
    else {
        MLOG_F(SIM, DBG, "erx: disabled_ic is set\n");
    }

    m_dma.intr.setRxState(DwmacState::RX_SUSPENDED);
    return true;
}

void Dwmac::tx_thread() {
    for(;;) {
        wait(m_tx_event);
        if (m_dma.intr.getTxState() != DwmacState::TX_STOPPED) {
            while(tx()); // consume all packets in the queue
        }
    }
}

void Dwmac::payload_recv(Payload &frame)
{
    MLOG_F(SIM, TRC, "received frame of length %u\n", (unsigned) frame.size());
    if(m_dma.intr.getRxState() != DwmacState::RX_STOPPED) {
        rx(frame);
    }
}

void Dwmac::intr_thread() {
    for (;;) {
        wait(intr_wake);
        intr0 = m_dma.intr.has_interrupts();
    }
}
