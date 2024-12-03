/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <arpa/inet.h>
#include <scp/report.h>

#include <dwmac.h>

using namespace sc_core;
using namespace std;

dwmac::dwmac(sc_module_name name)
    : sc_module(name)
    , intr0("mac-irq")
    , intr1("eth-wake-irq")
    , socket("socket")
    , dma_inst("dma")
    , phy_inst("phy", 0x00010001, 0)
    , m_tx_frame(9018u)
{
    reset();
    SCP_TRACE(SCMOD) << "Constructor";
    std::string mac = "0a:a0:db:c0:49:c3";
    SCP_TRACE(SCMOD) << mac;
    if (!m_mac_0.set_from_str(mac)) {
        SCP_ERR(SCMOD) << "Invalid mac-address '" << mac << "'";
    }
    SCP_TRACE(SCMOD) << "Mac-address initialized to " << std::hex << m_mac_0[0] << ":" << std::hex << m_mac_0[1] << ":"
                     << std::hex << m_mac_0[2] << ":" << std::hex << m_mac_0[3] << ":" << std::hex << m_mac_0[4] << ":"
                     << std::hex << m_mac_0[5];

    for (int i = 0; i < 8; i++) {
        m_htbl[i] = 0;
    }

    m_int_mask = 0;

    socket.register_b_transport(this, &dwmac::b_transport);

    SC_THREAD(tx_thread);
    SC_THREAD(intr_thread);
}

dwmac::~dwmac() {}

uint32_t dwmac::read_word(uint32_t address)
{
    if ((address & 0x03)) {
        SCP_DEBUG(SCMOD) << "requested address not aligned";
        return 0;
    }

    if (address >= DWMAC_MACn && address < DWMAC_MACn + (MACn_COUNT * 8)) {
        // Programmation of an extra MAC address
        bool islow = ((address - DWMAC_MACn) >> 2) & 0x01;
        std::size_t macindex = ((address - DWMAC_MACn) >> 3);

        assert(macindex < MACn_COUNT);
        if (islow)
            return m_mac_n[macindex].lo();
        else
            return m_mac_n[macindex].hi();
    }

    switch (address) {
    case DMA_BUS_MODE:
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

    case DWMAC_GMII_DATA: {
        uint16_t data = 0;
        bool err = true;
#if 0
            phy_inst.mdio_reg_read(m_gmii_address.pa, m_gmii_address.value, data, err);
#else
        if (m_gmii_address.pa == 0) {
            phy_inst.mdio_reg_read(m_gmii_address.gr, data);
            err = false;
        }
#endif
        if (err) {
            SCP_DEBUG(SCMOD) << "read: gmii PHY " << m_gmii_address.pa << " not present";
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
        SCP_ERR(SCMOD) << "WakeUp register no supported";
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
            SCP_WARN(SCMOD) << "read to A-N registers not supported (0x" << std::hex << (unsigned)address
                            << "))(will warn only once)";
            warn_anregs = false;
        } else {
            SCP_DEBUG(SCMOD) << "read to A-N registers not supported (0x" << std::hex << (unsigned)address << "))";
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

    case HW_FEATURE: {
        uint32_t hw_feature = 0x10D7F37; // Default value (from CycloneV datasheet)
        hw_feature &= ~(1 << 14);        // Energy Efficient Ethernet disabled (because it
                                         // triggers not yet supported MMD read)
        hw_feature |= (1 << 24);         // Enhanced Descriptor Select enabled
        return hw_feature;
    }

    default:
        SCP_DEBUG(SCMOD) << "invalid read at address 0x" << std::hex << address;
        return 0x00000000;
    }
}

void dwmac::write_word(uint32_t address, uint32_t data)
{
    dma_desc desc;

    if ((address & 0x03)) {
        SCP_DEBUG(SCMOD) << "requested address not aligned";
        return;
    }

    if (address >= DWMAC_MACn && address < DWMAC_MACn + (MACn_COUNT * 8)) {
        bool islow = ((address - DWMAC_MACn) >> 2) & 0x01;
        std::size_t macindex = ((address - DWMAC_MACn) >> 3);

        assert(macindex < MACn_COUNT);
        if (islow)
            m_mac_n[macindex].set_lo(data);
        else
            m_mac_n[macindex].set_hi(data);

        return;
    }

    switch (address) {
    case DMA_BUS_MODE:
        m_dma.bus_mode.value = data;
        SCP_DEBUG(SCMOD) << "Dma_BusMode: dsl=" << m_dma.bus_mode.dsl << ", atds=" << m_dma.bus_mode.atds;
        if (m_dma.bus_mode.swr) reset();
        return;

    case DMA_TX_POLL:
        read_tx_desc(&desc);
        if (!desc.des01.etx.own) {
            /* owned by host : suspend TX */
            m_dma.intr.m_dma_reg_status.tu = 1;
            m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
            m_tx_event.notify();
        } else {
            /* resume TX */
            m_dma.intr.setTxState(DwmacState::TX_SUSPENDED); // TODO: shoud be active state (but it is
                                                             // not yet modelized)
            m_tx_event.notify();
        }
        return;

    case DMA_RX_POLL:
        read_rx_desc(&desc);
        if (!desc.des01.erx.own) {
            /* owned by host : suspend RX */
            m_dma.intr.m_dma_reg_status.ru = 0;
            m_dma.intr.setRxState(DwmacState::RX_SUSPENDED);
        } else {
            /* resume RX */
            m_dma.intr.setRxState(DwmacState::RX_SUSPENDED); // TODO: shoud be active state (but it is
                                                             // not yet modelized)
                                                             // TODO: should wake up the 'wait(delay)'
        }
        return;

    case DMA_RX_DESC_LIST_ADDR:
        if (m_dma.intr.getRxState() != DwmacState::RX_STOPPED) {
            SCP_DEBUG(SCMOD) << "invalid write of RxDescrListAddr when Rx active";
        } else {
            m_dma.rx_desc_addr = data;
            m_dma.current_rx_desc = m_dma.rx_desc_addr;
        }
        return;

    case DMA_TX_DESC_LIST_ADDR:
        if (m_dma.intr.getTxState() != DwmacState::TX_STOPPED) {
            SCP_DEBUG(SCMOD) << "invalid write of TxDescrListAddr when Tx active";
        } else {
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
            if (m_dma.intr.getRxState() == DwmacState::RX_STOPPED) m_dma.intr.setRxState(DwmacState::RX_SUSPENDED);
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
        SCP_ERR(SCMOD) << "cannot write DMA_CURRENT_TX_DESC";
        return;

    case DMA_CURRENT_RX_DESC:
        SCP_ERR(SCMOD) << "cannot write DMA_CURRENT_RX_DESC";
        return;

    case DMA_CURRENT_TX_BUF:
        SCP_ERR(SCMOD) << "cannot write DMA_CURRENT_TX_BUF";
        return;

    case DMA_CURRENT_RX_BUF:
        SCP_ERR(SCMOD) << "cannot write DMA_CURRENT_RX_BUF";
        return;

    case DWMAC_CONFIGURATION:
        m_configuration = data;
        return;

    case DWMAC_FRAME_FILTER:
        m_frame_filter = data;
        return;

    case DWMAC_HASH_TABLE_HI:
        m_hash_table_hi = data;
        return;

    case DWMAC_HASH_TABLE_LO:
        m_hash_table_lo = data;
        return;

    case DWMAC_GMII_ADDRESS:
        m_gmii_address.value = data;
        m_gmii_address.gb = 0;
        return;

    case DWMAC_GMII_DATA: {
        bool err = true;
#if 0
            phy_inst.mdio_reg_write(m_gmii_address.pa, m_gmii_address.value, data, err);
#else
        if (m_gmii_address.pa == 0) {
            phy_inst.mdio_reg_write(m_gmii_address.gr, data);
            err = false;
        }
#endif
        if (err) {
            SCP_DEBUG(SCMOD) << "write: gmii PHY " << m_gmii_address.pa << " not present";
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
        SCP_DEBUG(SCMOD) << "version register is read-only";
        return;

    case DWMAC_WAKE_UP_FILTER:
        SCP_ERR(SCMOD) << "WakeUp register not supported";
        abort();

    case DWMAC_PMT:
        m_pmt = data;
        return;

    case DWMAC_INTERRUPT:
        SCP_DEBUG(SCMOD) << "interrupt status register is read-only";
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
        SCP_ERR(SCMOD) << "write to A-N registers not supported (0x" << std::hex << (unsigned)address << "))";
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
        SCP_DEBUG(SCMOD) << "invalid write at address 0x" << std::hex << address;
        return;
    }
}

void dwmac::interrupts_fire(uint32_t interrupts)
{
    m_dma.intr.fire(interrupts);
    intr_wake.notify();
}

void dwmac::reset()
{
    SCP_TRACE(SCMOD) << "Reset";

    m_dma.bus_mode.value = 0x00000000;
    m_dma.rx_desc_addr = 0x00000000;
    m_dma.tx_desc_addr = 0x00000000;
    m_dma.intr.reset();
    m_dma.opr_mode.value = 0x00000000;
    m_dma.counters = 0x00000000;
    m_dma.current_tx_desc = 0x00000000;
    m_dma.current_rx_desc = 0x00000000;
    m_dma.current_tx_buf = 0x00000000;
    m_dma.current_rx_buf = 0x00000000;
    m_configuration = 0x00000000;
    m_frame_filter = 0x00000000;
    m_hash_table_lo = 0x00000000;
    m_hash_table_hi = 0x00000000;
    m_gmii_address.value = 0x00000000;
    m_gmii_data = 0x00000000;
    m_flow_control = 0x00000000;
    m_vlan_tag = 0x00000000;
    m_pmt = 0x00000000;
    m_axi_bus_mode = 0x00110001;
    m_mac_0.zero();

    for (uint32_t i = 0; i < MACn_COUNT; ++i) m_mac_n[i].zero();
}

bool dwmac::read_tx_desc(dma_desc* desc)
{
    dma_inst.bus_read(m_dma.current_tx_desc, (uint8_t*)desc, m_dma.desc_size());

    if (!desc->des01.etx.own) return false;

    return true;
}

bool dwmac::read_rx_desc(dma_desc* desc)
{
    dma_inst.bus_read(m_dma.current_rx_desc, (uint8_t*)desc, m_dma.desc_size());

    // clear status ?
    // desc->des01.rx &= 0x80000000;

    if (!desc->des01.erx.own) return false;

    return true;
}

void dwmac::move_to_next_tx_desc(dma_desc& desc)
{
    SCP_TRACE(SCMOD) << "moving to the next TX descriptor";

    if (desc.des01.etx.end_ring) {
        SCP_TRACE(SCMOD) << "TX: ring : end reached, pointing back to the first entry";
        m_dma.current_tx_desc = m_dma.tx_desc_addr;
    } else {
        if (desc.des01.etx.second_address_chained) {
            m_dma.current_tx_desc = desc.des3;
        } else {
            if (desc.des01.etx.buffer2_size > 0 || desc.des3 != 0) {
                SCP_ERR(SCMOD) << "TX: second buffer not supported !";
                exit(1);
            }
            m_dma.current_tx_desc += m_dma.desc_size() + (m_dma.bus_mode.dsl * 4);
        }
    }
}

static uint16_t checksum(const uint8_t* buf, int size, uint32_t sum)
{
    int i;

    /* Accumulate checksum */
    for (i = 0; i < size - 1; i += 2) {
        sum += *(uint16_t*)&buf[i];
    }

    /* Handle odd-sized case */
    if (size & 1) {
        sum += buf[i];
    }

    /* Fold to get the ones-complement result */
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);

    /* Invert to get the negative in ones-complement arithmetic */
    return ~sum;
}

bool dwmac::tx()
{
    dma_desc desc;

    if (m_dma.intr.getTxState() != DwmacState::TX_SUSPENDED) {
        SCP_DEBUG(SCMOD) << "entering transmit routine without being in SUSP mode";
        return false;
    }

    m_dma.intr.setTxState(DwmacState::TX_FETCHING);

    if (!read_tx_desc(&desc)) {
        m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
        interrupts_fire(InterruptRegister::TxBufUnavailable);
        return false;
    }

    if (!desc.des01.etx.first_segment) {
        SCP_ERR(SCMOD) << "descriptor is not marked as being the first segment";
        m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
        return false;
    }

    m_dma.intr.setTxState(DwmacState::TX_READING);

    m_tx_frame.resize(desc.des01.etx.buffer1_size);
    dma_inst.bus_read(desc.des2, m_tx_frame.data(), desc.des01.etx.buffer1_size);

    // fetching the whole frame
    while (!desc.des01.etx.last_segment) {
        uint32_t cur_desc_addr = m_dma.current_tx_desc;

        move_to_next_tx_desc(desc);

        // disown descriptor
        desc.des01.etx.own = 0;
        dma_inst.bus_write(cur_desc_addr, (uint8_t*)&desc, m_dma.desc_size());

        // read next descriptor
        if (!read_tx_desc(&desc)) {
            SCP_ERR(SCMOD) << "cannot read descriptor";
            exit(1);
        }
        int offset = m_tx_frame.size();
        m_tx_frame.resize(offset + desc.des01.etx.buffer1_size);
        dma_inst.bus_read(desc.des2, m_tx_frame.data() + offset, desc.des01.etx.buffer1_size);
    }

    if (m_tx_frame.size() < 14) {
        SCP_DEBUG(SCMOD) << "ethernet buffer size too small";
        m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
        return false;
    }

    /* Checksum offloading */
    if ((m_configuration & (1 << 10)) && desc.des01.etx.checksum_insertion) {
        uint8_t* buffer = m_tx_frame.data();
        uint32_t len = m_tx_frame.size();

        if (len >= 14 && buffer[12] == 0x08 && buffer[13] == 0x00) {
            // IPv4 packet
            uint8_t* ipv4_pkt = &buffer[14];
            uint32_t ipv4_header_len = 4 * (ipv4_pkt[0] & 0x0f);

            /* IPv4 checksum */
            if (len >= (14 + ipv4_header_len)) {
                *(uint16_t*)(ipv4_pkt + 10) = 0;
                uint32_t csum = checksum(ipv4_pkt, ipv4_header_len, 0);
                // printf("Calculated checksum: %02x%02x\n", (csum >> 8) & 0xff, csum &
                // 0xff);
                *(uint16_t*)(ipv4_pkt + 10) = csum;
            }

            const uint8_t ip_prot = ipv4_pkt[9];

            /* TCP/UDP checksum */
            if ((desc.des01.etx.checksum_insertion == 3) && len >= 24 && (ip_prot == 0x06 || ip_prot == 0x11)) {
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

                uint8_t* tcp_seg = &ipv4_pkt[ipv4_header_len];
                // printf("Old checksum: %02X%02x\n", tcp_seg[16], tcp_seg[17]);

                /* Erase old checksum */
                *(uint16_t*)(tcp_seg + csum_offset) = 0x0000;

                uint32_t csum = 0;

                /* Pseudo header */
                csum += *(uint16_t*)(ipv4_pkt + 12);
                csum += *(uint16_t*)(ipv4_pkt + 14);
                csum += *(uint16_t*)(ipv4_pkt + 16);
                csum += *(uint16_t*)(ipv4_pkt + 18);
                csum += htons(ip_prot);
                csum += htons(len - 14 - ipv4_header_len);

                /* TCP/UDP segment */
                int size = len - 14 - ipv4_header_len;
                csum = checksum(tcp_seg, size, csum);
                // printf("Calculated checksum: %02x%02x\n", (csum >> 8) & 0xff, csum &
                // 0xff);

                *(uint16_t*)(tcp_seg + csum_offset) = csum;
            }
        }
    }

    SCP_TRACE(SCMOD) << "sending frame of length " << (unsigned)m_tx_frame.size();
    m_backend->send(m_tx_frame);

    m_dma.intr.setTxState(DwmacState::TX_CLOSING);

    // disown descriptor
    desc.des01.etx.own = 0;
    dma_inst.bus_write(m_dma.current_tx_desc, (uint8_t*)&desc, m_dma.desc_size());

    move_to_next_tx_desc(desc);

    if (desc.des01.etx.interrupt) interrupts_fire(InterruptRegister::TxInt);

    m_dma.intr.setTxState(DwmacState::TX_SUSPENDED);
    return true;
}

bool dwmac::rx(Payload& frame)
{
    if (frame.size() < 14) return false;

    dma_desc desc;

    if (!m_dma.opr_mode.sr) {
        SCP_DEBUG(SCMOD) << "DMA not started";
        return false;
    }

    if (m_dma.intr.getRxState() != DwmacState::RX_SUSPENDED) {
        SCP_DEBUG(SCMOD) << "entering receive routine without being in SUSP mode";
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
        SCP_DEBUG(SCMOD) << "RX buffer too small";
        return false;
    }

    dma_inst.bus_write(desc.des2, frame.data(), (int)frame.size());

    m_dma.intr.setRxState(DwmacState::RX_CLOSING);
    desc.des01.erx.first_descriptor = 1;
    desc.des01.erx.last_descriptor = 1;
    desc.des01.erx.frame_length = frame.size() + 4;

    desc.des01.erx.frame_type = 1;  // if LT > 0x0600 (for example 0x0806 for IP)
    desc.des01.erx.rx_mac_addr = 0; // extended status not available (we would need to implement it)

    desc.des01.erx.own = 0;
    dma_inst.bus_write(m_dma.current_rx_desc, (uint8_t*)&desc, m_dma.desc_size());

    SCP_TRACE(SCMOD) << "moving to the next RX descriptor";

    if (desc.des01.erx.end_ring) {
        SCP_TRACE(SCMOD) << "RX: ring : end reached, pointing back to the first entry";
        m_dma.current_rx_desc = m_dma.rx_desc_addr;
    } else {
        if (desc.des01.erx.second_address_chained) {
            m_dma.current_rx_desc = desc.des3;
        } else {
            m_dma.current_rx_desc += m_dma.desc_size() + (m_dma.bus_mode.dsl * 4);
        }
    }

    if (!desc.des01.erx.disable_ic) {
        interrupts_fire(InterruptRegister::RxInt);
    } else {
        SCP_DEBUG(SCMOD) << "erx: disabled_ic is set";
    }

    m_dma.intr.setRxState(DwmacState::RX_SUSPENDED);
    return true;
}

void dwmac::tx_thread()
{
    for (;;) {
        wait(m_tx_event);
        if (m_dma.intr.getTxState() != DwmacState::TX_STOPPED) {
            while (tx())
                ; // consume all packets in the queue
        }
    }
}

void dwmac::payload_recv(Payload& frame)
{
    SCP_TRACE(SCMOD) << "received frame of length " << (unsigned)frame.size();
    if (m_dma.intr.getRxState() != DwmacState::RX_STOPPED) {
        rx(frame);
    }
}

void dwmac::intr_thread()
{
    for (;;) {
        wait(intr_wake);
        intr0 = m_dma.intr.has_interrupts();
    }
}

void module_register() { GSC_MODULE_REGISTER_C(dwmac); }
