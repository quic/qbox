/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include <scp/report.h>
#include <systemc>

#include "backends/net-backend.h"
#include "components/dma.h"
#include "components/mac.h"
#include "components/mii.h"
#include "components/phy.h"

#include <deque>

union opr_mode_reg {
    struct {
        uint32_t reserved1 : 1;
        uint32_t sr : 1;
        uint32_t osf : 1;
        uint32_t rtc : 2;
        uint32_t reserved2 : 1;
        uint32_t fuf : 1;
        uint32_t fef : 1;
        uint32_t efc : 1;
        uint32_t rfa : 2;
        uint32_t rfd : 2;
        uint32_t st : 1;
        uint32_t ttc : 3;
        uint32_t reserved3 : 3;
        uint32_t ftf : 1;
        uint32_t tsf : 1;
        uint32_t reserved4 : 2;
        uint32_t dff : 1;
        uint32_t rsf : 1;
        uint32_t dt : 1;
        uint32_t reserved5 : 5;
    };
    uint32_t value;
};

union s_gmii_address {
    struct {
        uint32_t gb : 1;
        uint32_t gw : 1;
        uint32_t cr : 4;
        uint32_t gr : 5;
        uint32_t pa : 5;
    };
    uint32_t value;
};

union bus_mode_reg {
    struct {
        uint32_t swr : 1;
        uint32_t reserved1 : 1;
        uint32_t dsl : 5;
        uint32_t atds : 1;
        uint32_t pbl : 6;
        uint32_t reserved2 : 2;
        uint32_t fb : 1;
        uint32_t rpbl : 6;
        uint32_t usp : 1;
        uint32_t eightxpbl : 1;
        uint32_t aal : 1;
        uint32_t reserved3 : 6;
    };
    uint32_t value;
};

namespace DwmacState {
enum Tx { TX_STOPPED = 0, TX_FETCHING = 1, TX_WAITING = 2, TX_READING = 3, TX_SUSPENDED = 6, TX_CLOSING = 7 };
enum Rx { RX_STOPPED = 0, RX_FETCHING = 1, RX_WAITING = 3, RX_SUSPENDED = 4, RX_CLOSING = 5, RX_TRANSFERRING = 7 };
} // namespace DwmacState

union status_reg {
    struct {
        uint32_t ti : 1;
        uint32_t tps : 1;
        uint32_t tu : 1;
        uint32_t tjt : 1;
        uint32_t ovf : 1;
        uint32_t unf : 1;
        uint32_t ri : 1;
        uint32_t ru : 1;
        uint32_t rps : 1;
        uint32_t rwt : 1;
        uint32_t eti : 1;
        uint32_t reserved1 : 2;
        uint32_t fbi : 1;
        uint32_t eri : 1;
        uint32_t ais : 1;
        uint32_t nis : 1;
        DwmacState::Rx rs : 3;
        DwmacState::Tx ts : 3;
        uint32_t eb : 3;
        uint32_t gli : 1;
        uint32_t gmi : 1;
        uint32_t reserved2 : 1;
        uint32_t tti : 1;
        uint32_t glpii : 1;
        uint32_t reserved3 : 1;
    };
    uint32_t value;
};

class InterruptRegister
{
public:
    static const uint32_t NIS_mask = 0x4045;
    static const uint32_t AIS_mask = 0x27ba;
    static const uint32_t NIS_bit = 1 << 16;
    static const uint32_t AIS_bit = 1 << 15;
    static const uint32_t IS_mask = NIS_mask | AIS_mask;
    static const uint32_t IS_bits = NIS_bit | AIS_bit;
    static const uint32_t BE_mask = 7;

    enum IRQ {
        EarlyReceive = 1 << 14,
        BusError = 1 << 13,
        EarlyTransmit = 1 << 10,
        RxWatchdogTimeout = 1 << 9,
        RxStopped = 1 << 8,
        RxBufUnavailable = 1 << 7,
        RxInt = 1 << 6,
        Underflow = 1 << 5,
        Overflow = 1 << 4,
        TxJabberTimeout = 1 << 3,
        TxBufUnavailable = 1 << 2,
        TxStopped = 1 << 1,
        TxInt = 1 << 0
    };

    void setTxState(DwmacState::Tx state)
    {
        m_dma_reg_status.ts = state;
        m_regstate_staled = true;
    }

    void setRxState(DwmacState::Rx state)
    {
        m_dma_reg_status.rs = state;
        m_regstate_staled = true;
    }

    DwmacState::Tx getTxState() const { return m_dma_reg_status.ts; }
    DwmacState::Rx getRxState() const { return m_dma_reg_status.rs; }

    void reset()
    {
        m_regstate_staled = false;

        m_regstate = 0;
        m_regmask = 0;

        m_interrupts = 0;

        m_dma_reg_status.value = 0;
    }

    void clear(uint32_t clear)
    {
        clear &= IS_mask | IS_bits;

        if ((clear & NIS_mask) && !(clear & NIS_bit)) {
            SCP_INFO("dwmac.h") << "Clearing normal interrupts without clearing NIS";
        }

        if ((clear & AIS_mask) && !(clear & AIS_bit)) {
            SCP_INFO("dwmac.h") << "Clearing abnormal interrupts without clearing AIS";
        }

        m_interrupts &= ((~clear) & IS_mask);

        if ((m_interrupts & NIS_mask) && (m_regmask & NIS_bit)) m_interrupts |= NIS_bit;
        if ((m_interrupts & AIS_mask) && (m_regmask & AIS_bit)) m_interrupts |= AIS_bit;

        m_regstate_staled = true;
    }

    void fire(uint32_t interrupts)
    {
        interrupts &= m_regmask;
        interrupts &= IS_mask;

        if ((interrupts & NIS_mask) && (m_regmask & NIS_bit)) interrupts |= NIS_bit;
        if ((interrupts & AIS_mask) && (m_regmask & AIS_bit)) interrupts |= AIS_bit;
        m_interrupts |= interrupts;
        m_regstate_staled = true;
    }

    bool has_interrupts() const { return ((m_interrupts & m_regmask) != 0); }

    void set_interrupts(uint32_t mask) { m_regmask = (mask & (IS_mask | IS_bits)); }

    void set_status(uint32_t value) { clear(value & (IS_mask | IS_bits)); }

    uint32_t interrupts() const { return m_regmask; }

    uint32_t status() const
    {
        if (m_regstate_staled) {
            m_regstate = m_interrupts | m_dma_reg_status.value;

            m_regstate_staled = false;
        }
        return m_regstate;
    }

public:
    mutable bool m_regstate_staled;
    mutable uint32_t m_regstate;
    uint32_t m_regmask;

    uint32_t m_interrupts;
    union status_reg m_dma_reg_status;
};

/* Basic descriptor structure for normal and alternate descriptors */
struct dma_desc {
    /* Receive descriptor */
    union {
#if 0
        struct {
            /* RDES0 */
            uint32_t payload_csum_error:1;
            uint32_t crc_error:1;
            uint32_t dribbling:1;
            uint32_t mii_error:1;
            uint32_t receive_watchdog:1;
            uint32_t frame_type:1;
            uint32_t collision:1;
            uint32_t ipc_csum_error:1;
            uint32_t last_descriptor:1;
            uint32_t first_descriptor:1;
            uint32_t vlan_tag:1;
            uint32_t overflow_error:1;
            uint32_t length_error:1;
            uint32_t sa_filter_fail:1;
            uint32_t descriptor_error:1;
            uint32_t error_summary:1;
            uint32_t frame_length:14;
            uint32_t da_filter_fail:1;
            uint32_t own:1;
            /* RDES1 */
            uint32_t buffer1_size:11;
            uint32_t buffer2_size:11;
            uint32_t reserved1:2;
            uint32_t second_address_chained:1;
            uint32_t end_ring:1;
            uint32_t reserved2:5;
            uint32_t disable_ic:1;
        } rx;
#endif
        struct {
            /* RDES0 */
            uint32_t rx_mac_addr : 1;
            uint32_t crc_error : 1;
            uint32_t dribbling : 1;
            uint32_t error_gmii : 1;
            uint32_t receive_watchdog : 1;
            uint32_t frame_type : 1;
            uint32_t late_collision : 1;
            uint32_t ipc_csum_error : 1;
            uint32_t last_descriptor : 1;
            uint32_t first_descriptor : 1;
            uint32_t vlan_tag : 1;
            uint32_t overflow_error : 1;
            uint32_t length_error : 1;
            uint32_t sa_filter_fail : 1;
            uint32_t descriptor_error : 1;
            uint32_t error_summary : 1;
            uint32_t frame_length : 14;
            uint32_t da_filter_fail : 1;
            uint32_t own : 1;
            /* RDES1 */
            uint32_t buffer1_size : 13;
            uint32_t reserved1 : 1;
            uint32_t second_address_chained : 1;
            uint32_t end_ring : 1;
            uint32_t buffer2_size : 13;
            uint32_t reserved2 : 2;
            uint32_t disable_ic : 1;
        } erx; /* -- enhanced -- */

        /* Transmit descriptor */
#if 0
        struct {
            /* TDES0 */
            uint32_t deferred:1;
            uint32_t underflow_error:1;
            uint32_t excessive_deferral:1;
            uint32_t collision_count:4;
            uint32_t vlan_frame:1;
            uint32_t excessive_collisions:1;
            uint32_t late_collision:1;
            uint32_t no_carrier:1;
            uint32_t loss_carrier:1;
            uint32_t payload_error:1;
            uint32_t frame_flushed:1;
            uint32_t jabber_timeout:1;
            uint32_t error_summary:1;
            uint32_t ip_header_error:1;
            uint32_t time_stamp_status:1;
            uint32_t reserved1:13;
            uint32_t own:1;
            /* TDES1 */
            uint32_t buffer1_size:11;
            uint32_t buffer2_size:11;
            uint32_t time_stamp_enable:1;
            uint32_t disable_padding:1;
            uint32_t second_address_chained:1;
            uint32_t end_ring:1;
            uint32_t crc_disable:1;
            uint32_t checksum_insertion:2;
            uint32_t first_segment:1;
            uint32_t last_segment:1;
            uint32_t interrupt:1;
        } tx;
#endif
        struct {
            /* TDES0 */
            uint32_t deferred : 1;
            uint32_t underflow_error : 1;
            uint32_t excessive_deferral : 1;
            uint32_t collision_count : 4;
            uint32_t vlan_frame : 1;
            uint32_t excessive_collisions : 1;
            uint32_t late_collision : 1;
            uint32_t no_carrier : 1;
            uint32_t loss_carrier : 1;
            uint32_t payload_error : 1;
            uint32_t frame_flushed : 1;
            uint32_t jabber_timeout : 1;
            uint32_t error_summary : 1;
            uint32_t ip_header_error : 1;
            uint32_t time_stamp_status : 1;
            uint32_t reserved1 : 2;
            uint32_t second_address_chained : 1;
            uint32_t end_ring : 1;
            uint32_t checksum_insertion : 2;
            uint32_t reserved2 : 1;
            uint32_t time_stamp_enable : 1;
            uint32_t disable_padding : 1;
            uint32_t crc_disable : 1;
            uint32_t first_segment : 1;
            uint32_t last_segment : 1;
            uint32_t interrupt : 1;
            uint32_t own : 1;
            /* TDES1 */
            uint32_t buffer1_size : 13;
            uint32_t reserved3 : 3;
            uint32_t buffer2_size : 13;
            uint32_t reserved4 : 3;
        } etx; /* -- enhanced -- */
    } des01;
    unsigned int des2;
    unsigned int des3;

    /* if bus_mode.atds == 1 */
    unsigned int des4;
    unsigned int des5;
    unsigned int des6;
    unsigned int des7;
};

class Dwmac : public sc_core::sc_module
{
protected:
    bool warn_anregs = true;

    sc_core::sc_event m_tx_event;
    sc_core::sc_event intr_wake;

    void reset();

    void interrupts_fire(uint32_t interrupts);
    void interrupts_clear(uint32_t interrupts);

    bool read_tx_desc(dma_desc* desc);
    bool read_rx_desc(dma_desc* desc);

    bool tx();
    bool rx(Payload& frame);

    void move_to_next_tx_desc(dma_desc& desc);

    void tx_thread();
    void intr_thread();

    /* DMA registers */
    static const uint32_t DMA_BUS_MODE = 0x1000;
    static const uint32_t DMA_TX_POLL = 0x1004;
    static const uint32_t DMA_RX_POLL = 0x1008;
    static const uint32_t DMA_RX_DESC_LIST_ADDR = 0x100C;
    static const uint32_t DMA_TX_DESC_LIST_ADDR = 0x1010;
    static const uint32_t DMA_STATUS = 0x1014;
    static const uint32_t DMA_OPERATION_MODE = 0x1018;
    static const uint32_t DMA_INTERRUPTS = 0x101C;
    static const uint32_t DMA_COUNTERS = 0x1020;
    static const uint32_t DMA_RX_INT_WDT = 0x1024;
    static const uint32_t DMA_AXI_BUS_MODE = 0x1028;
    static const uint32_t DMA_CURRENT_TX_DESC = 0x1048;
    static const uint32_t DMA_CURRENT_RX_DESC = 0x104C;
    static const uint32_t DMA_CURRENT_TX_BUF = 0x1050;
    static const uint32_t DMA_CURRENT_RX_BUF = 0x1054;
    static const uint32_t HW_FEATURE = 0x1058;

    class DmaRegisters
    {
    public:
        union bus_mode_reg bus_mode;
        uint32_t rx_desc_addr;
        uint32_t tx_desc_addr;
        InterruptRegister intr;
        union opr_mode_reg opr_mode;
        uint32_t counters;
        uint32_t current_tx_desc;
        uint32_t current_rx_desc;
        uint32_t current_tx_buf;
        uint32_t current_rx_buf;

        uint32_t desc_size() { return (bus_mode.atds == 0) ? 16 : 32; }
    };

    DmaRegisters m_dma;

    /* DWMAC registers */
    static const uint32_t DWMAC_VERSION_VALUE = 0x0010;

    static const uint32_t DWMAC_CONFIGURATION = 0x0000;
    static const uint32_t DWMAC_FRAME_FILTER = 0x0004;
    static const uint32_t DWMAC_HASH_TABLE_HI = 0x0008;
    static const uint32_t DWMAC_HASH_TABLE_LO = 0x000C;
    static const uint32_t DWMAC_GMII_ADDRESS = 0x0010;
    static const uint32_t DWMAC_GMII_DATA = 0x0014;
    static const uint32_t DWMAC_FLOW_CONTROL = 0x0018;
    static const uint32_t DWMAC_VLAN_TAG = 0x001C;
    static const uint32_t DWMAC_VERSION = 0x0020;
    static const uint32_t DWMAC_WAKE_UP_FILTER = 0x0028;
    static const uint32_t DWMAC_PMT = 0x002C;
    static const uint32_t DWMAC_INTERRUPT = 0x0038;
    static const uint32_t DWMAC_INTERRUPT_MASK = 0x003C;
    static const uint32_t DWMAC_AN_CONTROL = 0x00C0;
    static const uint32_t DWMAC_AN_STATUS = 0x00C4;
    static const uint32_t DWMAC_AN_ADVERTISEMENT = 0x00C8;
    static const uint32_t DWMAC_AN_LINK_PARTNER_ABILITY = 0x00CC;
    static const uint32_t DWMAC_AN_EXPANSION = 0x00D0;
    static const uint32_t DWMAC_MAC0_HI = 0x0040;
    static const uint32_t DWMAC_MAC0_LO = 0x0044;
    static const uint32_t DWMAC_MACn = 0x0048;
    static const uint32_t DWMAC_MMC_CONTROL = 0x0100;
    static const uint32_t DWMAC_MMC_RX_INT = 0x0104;
    static const uint32_t DWMAC_MMC_TX_INT = 0x0108;
    static const uint32_t DWMAC_MMC_RX_INT_MASK = 0x010C;
    static const uint32_t DWMAC_MMC_TX_INT_MASK = 0x0110;
    static const uint32_t DWMAC_MMC_IPC_RX_INT_MASK = 0x0200;
    static const uint32_t DWMAC_MMC_IPC_RX_INT = 0x0208;

    static const uint32_t MACn_COUNT = 15;

    uint32_t m_configuration;
    uint32_t m_frame_filter;
    uint32_t m_hash_table_hi;
    uint32_t m_hash_table_lo;
    union s_gmii_address m_gmii_address;
    uint32_t m_gmii_data;
    uint32_t m_flow_control;
    uint32_t m_vlan_tag;
    uint32_t m_pmt;
    MACAddress m_mac_0;
    MACAddress m_mac_n[MACn_COUNT];
    uint32_t m_htbl[8];
    uint32_t m_int_mask;
    uint32_t m_axi_bus_mode;

public:
    sc_core::sc_out<bool> intr0, intr1;
    tlm_utils::simple_target_socket<Dwmac> socket;
    Dma dma;
    Phy phy;

    NetworkBackend* m_backend;
    void set_backend(NetworkBackend* backend)
    {
        m_backend = backend;
        m_backend->register_receive(this, eth_rx_sc, eth_can_rx_sc);
    }

    static void eth_rx_sc(void* opaque, Payload& frame) { ((Dwmac*)opaque)->payload_recv(frame); }

    static int eth_can_rx_sc(void* opaque) { return 1; }

    Payload m_tx_frame;

    SC_HAS_PROCESS(Dwmac);

    Dwmac(sc_core::sc_module_name name);
    virtual ~Dwmac();

    uint32_t read_word(uint32_t address);
    void write_word(uint32_t address, uint32_t data);

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        unsigned char* ptr = trans.get_data_ptr();
        uint64_t addr = trans.get_address();

        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            write_word(addr, *(uint32_t*)ptr);
            break;
        case tlm::TLM_READ_COMMAND:
            *(uint32_t*)ptr = read_word(addr);
            break;
        default:
            break;
        }
    }

    virtual void payload_recv(Payload& frame);
};
