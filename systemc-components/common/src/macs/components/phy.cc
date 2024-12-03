/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __APPLE__
#include <net/if_utun.h> // UTUN_CONTROL_NAME
#else
#include <linux/if_tun.h>
#endif
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <scp/report.h>

#include <macs/mii.h>
#include <macs/phy.h>

using namespace sc_core;

phy::phy(sc_core::sc_module_name name, uint32_t id, uint8_t addr): sc_module(name), m_id(id), m_addr(addr)
{
    SCP_TRACE(SCMOD) << "Constructor";
    if (m_addr >= 32) {
        SCP_ERR(SCMOD) << "Invalid address '" << m_addr << "', not in [0:31] range";
    }

    m_ctl = 0;
    m_status = BMSR_100FULL | BMSR_ANEGCOMPLETE | BMSR_LSTATUS;
    m_adv = ADVERTISE_ALL;
}

phy::~phy() {}

void phy::mdio_reg_read(uint8_t reg, uint16_t& data)
{
    SCP_TRACE(SCMOD) << "phy.read @" << reg;

    uint16_t res = 0x0000;
    switch (reg) {
    case MII_BMCR:
        res = m_ctl;
        break;
    case MII_BMSR:
        res = m_status;
        break;
    case MII_PHYSID1:
        res = m_id >> 16; // Phy ID hi
        break;
    case MII_PHYSID2:
        res = m_id & 0xffff; // Phy ID lo
        break;
    case MII_ADVERTISE:
        res = m_adv;
        break;
    case MII_LPA:
        res = LPA_LPACK | LPA_100FULL | LPA_100HALF | LPA_10FULL | LPA_10HALF | SLCT_IEEE802_3;
        break;
    case MII_MMD_CTRL:
        res = 0;
        break;
    case MII_MMD_DATA:
        res = 0;
        break;
    }
    data = res;
}

void phy::mdio_reg_write(uint8_t reg, uint16_t value)
{
    SCP_TRACE(SCMOD) << "phy::write @ " << reg << " value=0x" << std::hex << value;

    switch (reg) {
    case MII_BMCR:
        if (value & BMCR_ANENABLE) {
            SCP_DEBUG(SCMOD) << "mii: enabling auto negotiation";
        }
        if (value & BMCR_ANRESTART) {
            SCP_DEBUG(SCMOD) << "mii: starting auto negotiation";
        }
        if (value & BMCR_RESET) {
            SCP_DEBUG(SCMOD) << "mii: reset !";
        }
        if (value & BMCR_PDOWN) {
            SCP_DEBUG(SCMOD) << "mii: power down !";
        }

        m_ctl = value;
        m_ctl &= ~BMCR_RESET;
        m_ctl &= ~BMCR_ANRESTART;
        m_ctl &= ~BMCR_PDOWN;

        break;
    case MII_ADVERTISE:
        m_adv = value;
        break;
    case MII_MMD_CTRL: {
#if 0
            uint16_t fcn = (value >> 14) & 0x0003;
            uint16_t devad = value & 0x001F;
            SCP_DEBUG(SCMOD) << "MMD acces control: function=0x" << std::hex << fcn << ", devad=0x" << std::hex << devad;
#endif
        break;
    }
    case MII_MMD_DATA: {
#if 0
            uint16_t addr_data = (value >> 14) & 0x0003;
            SCP_DEBUG(SCMOD) << "MMD acces address data: addr_data=0x" << std::hex << addr_data;
#endif
        break;
    }
    }
}
