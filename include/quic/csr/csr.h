/*
 * QEMU DSP CSR
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#define CSR_LOG                    \
    if (std::getenv("GS_LOG")) \
    std::cout

#define QDSP6SS_NMI 0x00040
#define QDSP6SS_BOOT_CORE_START 0x00400
#define QDSP6SS_BOOT_CMD 0x00404

#define NMI_CLEAR_STATUS_MASK 0x02
#define NMI_SET_MASK 0x01
#define BOOT_CORE_MASK 0x01
#define BOOT_CMD_MASK 0x01

#define QDSP6SS_VERSION 0x00000
#define QDSP6SS_RET_CFG 0x0001C
#define QDSP6SS_NMI_STATUS 0x00044
#define QDSP6SS_STRAP_TCM_BASE_STATUS 0x00100
#define QDSP6SS_STRAP_AHBUPPER_STATUS 0x00104
#define QDSP6SS_STRAP_AHBLOWER_STATUS 0x00108
#define QDSP6SS_STRAP_AHBS_BASE_STATUS 0x0010C
#define QDSP6SS_STRAP_CLADE_RWCFG_BASE_STATUS 0x00110
#define QDSP6SS_STRAP_AXIM2UPPER_STATUS 0x0011C
#define QDSP6SS_STRAP_AXIM2LOWER_STATUS 0x00120
#define QDSP6SS_DBG_NMI_PWR_STATUS 0x00304
#define QDSP6SS_BOOT_STATUS 0x00408
#define QDSP6SS_MEM_STATUS 0x00438
#define QDSP6SS_L2MEM_EFUSE_STATUS 0x00490
#define QDSP6SS_CPMEM_STATUS 0x00528
#define QDSP6SS_L2ITCM_STATUS 0x00538
#define QDSP6SS_LMH_STATUS 0x0081C
#define QDSP6SS_ISENSE_STATUS 0x00830
#define QDSP6SS_TEST_BUS_VALUE 0x02004
#define QDSP6SS_HMX_STATUS 0x02024
#define QDSP6SS_CORE_STATUS 0x02028
#define QDSP6SS_INTF_HALTACK 0x0208C
#define QDSP6SS_INTFCLAMP_STATUS 0x02098
#define QDSP6SS_CORE_BHS_STATUS 0x02418
#define QDSP6SS_RESET_STATUS 0x02448
#define QDSP6SS_CLAMP_STATUS 0x02458
#define QDSP6SS_CLK_STATUS 0x02468
#define QDSP6SS_MEM_STAGGER_RESET_STATUS 0x02478

#define PUBCSR_TRIG_MASK 0x1

class csr : public sc_core::sc_module
{
private:
    bool nmi_gen=false;
    bool nmi_clear_status=false;
    bool nmi_triggered_csr=false;
    bool boot_core_start=false;
    bool boot_status=false;
    sc_core::sc_event update_ev;
    uint64_t ret_cfg;

public:
    InitiatorSignalSocket<bool> hex_halt;
    InitiatorSignalSocket<bool> nmi;
    tlm_utils::simple_target_socket<csr> socket;
private:
    void csr_update()
    {
        if (nmi_gen)
        {
            nmi_triggered_csr = true;
            nmi->write(1);
            nmi->write(0);
        }
        if (nmi_clear_status)
        {
            nmi_triggered_csr = false;
        }

        hex_halt->write(!boot_status);
    }

    void csr_write(uint64_t offset, uint64_t val, unsigned size)
    {
        switch (offset)
        {
        case QDSP6SS_NMI:
            nmi_clear_status = (val & NMI_CLEAR_STATUS_MASK);
            nmi_gen = (val & NMI_SET_MASK);
            break;
        case QDSP6SS_BOOT_CORE_START:
            boot_core_start = (val & BOOT_CORE_MASK);
            break;
        case QDSP6SS_BOOT_CMD:
            boot_status = (val & BOOT_CMD_MASK);
            break;
        case QDSP6SS_RET_CFG:
            ret_cfg=val;
            break;
        default:
            SC_REPORT_WARNING("CSR", "Unimplemented write");
            break;
        }

        update_ev.notify();
    }


    uint64_t csr_read(uint64_t offset, unsigned size)
    {
        switch (offset)
        {
            /* RO registers: */
        case QDSP6SS_VERSION:
            return 0x10020000;
        case QDSP6SS_RET_CFG:
            return ret_cfg;
        case QDSP6SS_NMI_STATUS:
            return nmi_triggered_csr ? PUBCSR_TRIG_MASK : 0x0;
        case QDSP6SS_STRAP_TCM_BASE_STATUS:
            return 0x0;
        case QDSP6SS_STRAP_AHBUPPER_STATUS:
            return 0x0;
        case QDSP6SS_STRAP_AHBLOWER_STATUS:
            return 0x0;
        case QDSP6SS_STRAP_AHBS_BASE_STATUS:
            return 0x0;
        case QDSP6SS_STRAP_CLADE_RWCFG_BASE_STATUS:
            return 0x0;
        case QDSP6SS_STRAP_AXIM2UPPER_STATUS:
            return 0x0;
        case QDSP6SS_STRAP_AXIM2LOWER_STATUS:
            return 0x0;
        case QDSP6SS_DBG_NMI_PWR_STATUS:
            return 0x0;
        case QDSP6SS_BOOT_STATUS:
            return boot_status?0x1:0x0;
        case QDSP6SS_MEM_STATUS:
            return 0x1F001F;
        case QDSP6SS_L2MEM_EFUSE_STATUS:
            return 0xF;
        case QDSP6SS_CPMEM_STATUS:
            return 0x0;
        case QDSP6SS_L2ITCM_STATUS:
            return 0x0;
        case QDSP6SS_LMH_STATUS:
            return 0x1;
        case QDSP6SS_ISENSE_STATUS:
            return 0x0;
        case QDSP6SS_TEST_BUS_VALUE:
            return 0x0;
        case QDSP6SS_HMX_STATUS:
            return 0x14;
        case QDSP6SS_CORE_STATUS:
            return 0x0;
        case QDSP6SS_INTF_HALTACK:
            return 0x0;
        case QDSP6SS_INTFCLAMP_STATUS:
            return 0x0;
        case QDSP6SS_CORE_BHS_STATUS:
            return 0x0;
        case QDSP6SS_RESET_STATUS:
            return 0x0;
        case QDSP6SS_CLAMP_STATUS:
            return 0x0;
        case QDSP6SS_CLK_STATUS:
            return 0x1FFF;
        case QDSP6SS_MEM_STAGGER_RESET_STATUS:
            return 0x0;
        default:
            SC_REPORT_WARNING("CS", "Unimplemented read");
            return 0x0;
        }
        return 0;
    }

    void csr_reset()
    {
        nmi_gen = false;
        nmi_clear_status = false;
        nmi_triggered_csr = false;
        boot_core_start = false;
        boot_status = false;

        csr_update();
    }

    void b_transport(tlm::tlm_generic_payload &txn, sc_core::sc_time &delay)
    {
        unsigned int len = txn.get_data_length();
        unsigned char *ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        assert(len == 4);

        switch (txn.get_command())
        {
        case tlm::TLM_READ_COMMAND:
        {
            uint32_t data = csr_read(addr & 0xfff, len);
            memcpy(ptr, &data, len);
            CSR_LOG << "csr : b_transport read " << std::hex << addr << " " << data << "\n";
            break;
        }
        case tlm::TLM_WRITE_COMMAND:
        {
            uint32_t data;
            memcpy(&data, ptr, len);
            CSR_LOG << "csr : b_transport write " << std::hex << addr << " " << data << "\n";
            csr_write(addr & 0xfff, data, len);
            break;
        }
        default:
            SC_REPORT_ERROR("CSR", "TLM command not supported\n");
            break;
        }

        txn.set_response_status(tlm::TLM_OK_RESPONSE);
    }

public:
    SC_HAS_PROCESS(csr);
    csr(sc_core::sc_module_name name)
        : socket("socket")
    {
        socket.register_b_transport(this, &csr::b_transport);
        SC_THREAD(csr_reset);

        SC_METHOD(csr_update);
        sensitive << update_ev;
    }
};
