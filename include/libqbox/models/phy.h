/*
 *  This file is part of libqbox
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

#pragma once

#include <sys/types.h>
#include <inttypes.h>

#include <systemc>

#include "libqbox/backends/net-backend.h"
#include "link.h"

class Phy : public sc_core::sc_module {
private:

protected:
    uint16_t m_ctl;
    uint16_t m_status;
    uint16_t m_adv;

public:
    SC_HAS_PROCESS(Phy);

    const uint32_t m_id;
    const uint8_t m_addr;

    Phy(sc_core::sc_module_name name, uint32_t id, uint8_t addr);
    virtual ~Phy();

    virtual void mdio_reg_read(uint8_t reg, uint16_t &data);
    virtual void mdio_reg_write(uint8_t reg, uint16_t data);
};
