/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <systemc>

#include "../backends/net-backend.h"
//#include "link.h"

class Phy : public sc_core::sc_module
{
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

    virtual void mdio_reg_read(uint8_t reg, uint16_t& data);
    virtual void mdio_reg_write(uint8_t reg, uint16_t data);
};
