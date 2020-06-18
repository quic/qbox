/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Greensocs
 *
 *  Author: Damien Hedde
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

#include <queue>
#include <mutex>

#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

#include "libqbox/backends/char-backend.h"

#include <libssync/async-event.h>

class IbexUart : public sc_core::sc_module {
public:
    enum Reg {
        REG_CTRL = 0x0c,
        REG_STATUS = 0x10,
        REG_WDATA = 0x18,
    };

    enum {
        FIELD_CTRL_TXENABLE = 0x1,
        FIELD_STATUS_TXEMPTY = 0x4,
    };

private:
    uint32_t ctrl;
    uint32_t status;

    CharBackend *chr;

public:
    tlm_utils::simple_target_socket<IbexUart> socket;

    sc_core::sc_vector<sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS>> irq;

    sc_core::sc_event update_event;

    SC_HAS_PROCESS(IbexUart);
    IbexUart(sc_core::sc_module_name name)
    {
        ctrl = 0;
        status = FIELD_STATUS_TXEMPTY;

        socket.register_b_transport(this, &IbexUart::b_transport);
    }

    void set_backend(CharBackend *backend)
    {
        chr = backend;
    }

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        unsigned char *ptr = trans.get_data_ptr();
        uint64_t addr = trans.get_address();

        if (trans.get_data_length() != 4) {
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        } else if (addr & 0x3) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            reg_write(addr, *(uint32_t *)ptr);
            break;
        case tlm::TLM_READ_COMMAND:
            *(uint32_t *)ptr = reg_read(addr);
            break;
        default:
            break;
        }

        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    uint64_t reg_read(uint64_t addr)
    {
        uint64_t r = 0;

        switch (addr) {
        case REG_CTRL:
            r = ctrl;
            break;
        case REG_STATUS:
            r = status;
            break;
        case REG_WDATA:
            break;
        }
        return r;
    }

    void reg_write(uint64_t addr, uint32_t data)
    {
        switch (addr) {
        case REG_CTRL:
            ctrl = data;
            break;
        case REG_STATUS:
            break;
        case REG_WDATA:
            if (chr) {
                chr->write(data);
            }
            break;
        }
    }
};
