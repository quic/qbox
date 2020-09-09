#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "models/phy.h"
#include "utils/mii.h"

#if 0
#define MLOG(foo, bar) \
    std::cout << "[" << name() << "] "
#define MLOG_F(foo, bar, args...)\
    printf(args)
#define LOG_F(foo, bar, args...)\
    printf(args)
#else
#include <fstream>
#define MLOG(foo, bar) \
    if (0) std::cout
#define MLOG_F(foo, bar, args...)
#define LOG_F(foo, bar, args...)
#endif

using namespace sc_core;

Phy::Phy(sc_core::sc_module_name name, uint32_t id, uint8_t addr)
    : sc_module(name)
    , m_id(id)
    , m_addr(addr)
{
    if (m_addr >= 32) {
        LOG_F(APP, ERR, "Invalid address '%u', not in [0:31] range\n", m_addr);
    }

    m_ctl = 0;
    m_status = BMSR_100FULL | BMSR_ANEGCOMPLETE | BMSR_LSTATUS;
    m_adv = ADVERTISE_ALL;
}

Phy::~Phy() {
}

void Phy::mdio_reg_read(uint8_t reg, uint16_t &data) {
    LOG_F(SIM, TRC, "Phy.read @ %u\n", reg);

    uint16_t res = 0x0000;
    switch(reg) {
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
        res = LPA_LPACK
            | LPA_100FULL | LPA_100HALF
            | LPA_10FULL | LPA_10HALF
            | SLCT_IEEE802_3;
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

void Phy::mdio_reg_write(uint8_t reg, uint16_t value) {
    LOG_F(SIM, TRC, "Phy::write @ %u value=0x%04x\n", reg, value);

    switch(reg) {
    case MII_BMCR:
        if(value & BMCR_ANENABLE) {
            LOG_F(SIM, DBG, "mii: enabling auto negotiation\n");
        }
        if(value & BMCR_ANRESTART) {
            LOG_F(SIM, DBG, "mii: starting auto negotiation\n");
        }
        if(value & BMCR_RESET) {
            LOG_F(SIM, DBG, "mii: reset !\n");
        }
        if(value & BMCR_PDOWN) {
            LOG_F(SIM, DBG, "mii: power down !\n");
        }

        m_ctl = value;
        m_ctl &= ~BMCR_RESET;
        m_ctl &= ~BMCR_ANRESTART;
        m_ctl &= ~BMCR_PDOWN;

        break;
    case MII_ADVERTISE:
        m_adv = value;
        break;
    case MII_MMD_CTRL:
        {
#if 0
            uint16_t fcn = (value >> 14) & 0x0003;
            uint16_t devad = value & 0x001F;
            LOG_F(SIM, DBG, "MMD acces control: function=0x%04x, devad=0x%04x\n", fcn, devad);
#endif
            break;
        }
    case MII_MMD_DATA:
        {
#if 0
            uint16_t addr_data = (value >> 14) & 0x0003;
            LOG_F(SIM, DBG, "MMD acces address data: addr_data=0x%04x\n", addr_data);
#endif
            break;
        }
    }
}
