/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <queue>
#include <mutex>

#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

#include "backends/char-backend.h"

#include <greensocs/libgssync/async_event.h>

#include <scp/report.h>

class IbexUart : public sc_core::sc_module
{
public:
    enum Reg {
        REG_ITSTAT = 0x00,
        REG_ITEN = 0x04,
        REG_ITTEST = 0x08,
        REG_CTRL = 0x0c,
        REG_STATUS = 0x10,
        REG_RDATA = 0x14,
        REG_WDATA = 0x18,
    };

    enum {
        FIELD_IT_RXWATERMARK = 0x2,
        FIELD_CTRL_TXENABLE = 0x1,
        FIELD_CTRL_RXENABLE = 0x2,
        FIELD_STATUS_TXEMPTY = 0x04,
        FIELD_STATUS_RXIDLE = 0x10,
        FIELD_STATUS_RXEMPTY = 0x20,
    };

    sc_core::sc_out<bool> irq_rxwatermark;

    uint32_t it_state;
    uint32_t it_enable;
    uint32_t ctrl;
    uint32_t status;

    uint8_t rdata;

    CharBackend* chr;

    static int can_receive(void* opaque)
    {
        IbexUart* ptr = (IbexUart*)opaque;

        return ptr->ctrl & FIELD_CTRL_RXENABLE;
    }

    void update_irqs(void)
    {
        uint32_t it = it_state & it_enable;
        if (it & FIELD_IT_RXWATERMARK) {
            irq_rxwatermark = true;
        } else {
            irq_rxwatermark = false;
        }
    }

    void recv(const uint8_t* buf, int size)
    {
        if ((ctrl & FIELD_CTRL_RXENABLE) && (status & FIELD_STATUS_RXEMPTY)) {
            rdata = *buf;
            status &= ~(FIELD_STATUS_RXEMPTY | FIELD_STATUS_RXIDLE);
            it_state |= FIELD_IT_RXWATERMARK;
            update_event.notify();
        } else {
            SCP_ERR(SCMOD) << "IbexUart: rx char overflow, ignoring chracter 0x" << (unsigned)*buf << " '" << *buf
                           << "'";
        }
    }

    static void receive(void* opaque, const uint8_t* buf, int size)
    {
        IbexUart* ptr = (IbexUart*)opaque;

        ptr->recv(buf, size);
    }

public:
    tlm_utils::simple_target_socket<IbexUart> socket;

    sc_core::sc_event update_event;

    SC_HAS_PROCESS(IbexUart);
    IbexUart(sc_core::sc_module_name name): irq_rxwatermark("irq_rx_watermark")
    {
        ctrl = 0;
        status = FIELD_STATUS_TXEMPTY | FIELD_STATUS_RXIDLE | FIELD_STATUS_RXEMPTY;
        rdata = 0;
        it_enable = 0;
        it_state = 0;

        SCP_DEBUG(SCMOD) << "IbexUart constructor";
        socket.register_b_transport(this, &IbexUart::b_transport);

        SC_METHOD(update_irqs);
        sensitive << update_event;
    }

    void set_backend(CharBackend* backend)
    {
        chr = backend;
        chr->register_receive(this, receive, can_receive);
    }

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        unsigned char* ptr = trans.get_data_ptr();
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
            reg_write(addr, *(uint32_t*)ptr);
            break;
        case tlm::TLM_READ_COMMAND:
            *(uint32_t*)ptr = reg_read(addr);
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
        case REG_ITSTAT:
            r = it_state;
            break;
        case REG_ITEN:
            r = it_enable;
            break;
        case REG_CTRL:
            r = ctrl;
            break;
        case REG_STATUS:
            r = status;
            break;
        case REG_RDATA:
            r = rdata;
            if (ctrl & FIELD_CTRL_RXENABLE) {
                status |= FIELD_STATUS_RXIDLE | FIELD_STATUS_RXEMPTY;
                rdata = 0;
            }
            break;
        }
        return r;
    }

    void reg_write(uint64_t addr, uint32_t data)
    {
        switch (addr) {
        case REG_ITSTAT:
            it_state &= ~data;
            update_event.notify();
            break;
        case REG_ITEN:
            it_enable = data & FIELD_IT_RXWATERMARK;
            update_event.notify();
            break;
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

    void before_end_of_elaboration()
    {
        if (!irq_rxwatermark.get_interface()) {
            sc_core::sc_signal<bool>* stub = new sc_core::sc_signal<bool>(sc_core::sc_gen_unique_name("stub"));
            irq_rxwatermark.bind(*stub);
        }
    }
};
