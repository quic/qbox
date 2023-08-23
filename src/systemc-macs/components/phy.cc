/*
 * Copyright (c) 2022 GreenSocs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

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

#include "greensocs/systemc-macs/components/mii.h"
#include "greensocs/systemc-macs/components/phy.h"

using namespace sc_core;

Phy::Phy(sc_core::sc_module_name name, uint32_t id, uint8_t addr): sc_module(name), m_id(id), m_addr(addr)
{
    SCP_TRACE(SCMOD) << "Constructor";
    if (m_addr >= 32) {
        SCP_ERR(SCMOD) << "Invalid address '" << m_addr << "', not in [0:31] range";
    }

    m_ctl = 0;
    m_status = BMSR_100FULL | BMSR_ANEGCOMPLETE | BMSR_LSTATUS;
    m_adv = ADVERTISE_ALL;
}

Phy::~Phy() {}

void Phy::mdio_reg_read(uint8_t reg, uint16_t& data)
{
    SCP_TRACE(SCMOD) << "Phy.read @" << reg;

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

void Phy::mdio_reg_write(uint8_t reg, uint16_t value)
{
    SCP_TRACE(SCMOD) << "Phy::write @ " << reg << " value=0x" << std::hex << value;

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
