/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <mutex>
#include <queue>

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include <systemc>

#include <ports/initiator-signal-socket.h>
#include <async_event.h>
#include <ports/biflow-socket.h>
#include <module_factory_registery.h>
#include <scp/report.h>
#include <tlm_sockets_buswidth.h>

#define PL011_INT_TX 0x20
#define PL011_INT_RX 0x10

#define PL011_FLAG_TXFE 0x80
#define PL011_FLAG_RXFF 0x40
#define PL011_FLAG_TXFF 0x20
#define PL011_FLAG_RXFE 0x10

/* Interrupt status bits in UARTRIS, UARTMIS, UARTIMSC */
#define INT_OE  (1 << 10)
#define INT_BE  (1 << 9)
#define INT_PE  (1 << 8)
#define INT_FE  (1 << 7)
#define INT_RT  (1 << 6)
#define INT_TX  (1 << 5)
#define INT_RX  (1 << 4)
#define INT_DSR (1 << 3)
#define INT_DCD (1 << 2)
#define INT_CTS (1 << 1)
#define INT_RI  (1 << 0)
#define INT_E   (INT_OE | INT_BE | INT_PE | INT_FE)
#define INT_MS  (INT_RI | INT_DSR | INT_DCD | INT_CTS)

typedef struct PL011State {
    uint32_t readbuff;
    uint32_t flags;
    uint32_t lcr;
    uint32_t rsr;
    uint32_t cr;
    uint32_t dmacr;
    uint32_t int_enabled;
    uint32_t int_level;
    uint32_t read_fifo[16];
    uint32_t ilpr;
    uint32_t ibrd;
    uint32_t fbrd;
    uint32_t ifl;
    int read_pos;
    int read_count;
    int read_trigger;
    const unsigned char* id;
} PL011State;

static const uint32_t irqmask[] = {
    INT_E | INT_MS | INT_RT | INT_TX | INT_RX, /* combined IRQ */
    INT_RX,
    INT_TX,
    INT_RT,
    INT_MS,
    INT_E,
};

class Pl011;

/* for legacy */
typedef Pl011 Uart;

class Pl011 : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    PL011State* s;

    tlm_utils::simple_target_socket<Pl011, DEFAULT_TLM_BUSWIDTH> socket;

    gs::biflow_socket<Pl011> backend_socket;

    InitiatorSignalSocket<bool> irq;

    sc_core::sc_event update_event;

    SC_HAS_PROCESS(Pl011);
    Pl011(sc_core::sc_module_name name)
        : irq("irq"), s(nullptr), socket("target_socket"), backend_socket("backend_socket")
    {
        SCP_TRACE(()) << "Pl011 constructor";

        socket.register_b_transport(this, &Pl011::b_transport);
        backend_socket.register_b_transport(this, &Pl011::pl011_receive);

        s = new PL011State();

        s->read_trigger = 1;
        s->ifl = 0x12;
        s->cr = 0x300;
        s->flags = 0x90;

        static const unsigned char pl011_id_arm[8] = { 0x11, 0x10, 0x14, 0x00, 0x0d, 0xf0, 0x05, 0xb1 };

        s->id = pl011_id_arm;

        SC_METHOD(pl011_update_sysc);
        sensitive << update_event;
    }

#if 0
    void write(uint64_t offset, uint32_t value) {
        switch (offset >> 2) {
        case 0:
            putchar(value);
            /* FIXME: flush on '\n' or a timeout */
            if (value == '\r' || value == '\n') {
                fflush(stdout);
            }
            break;
        default:
            break;
        }
    }
#endif
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        unsigned char* ptr = trans.get_data_ptr();
        uint64_t addr = trans.get_address();
        unsigned int len = trans.get_data_length();

        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            switch (len) {
            case 1:
                pl011_write(addr, *ptr);
                break;
            case 2:
                pl011_write(addr, *(uint16_t*)ptr);
                break;
            case 4:
                pl011_write(addr, *(uint32_t*)ptr);
                break;
            default:
                SCP_FATAL(()) << "Incorrect transaction size";
                break;
            }
            break;
        case tlm::TLM_READ_COMMAND:
            switch (len) {
            case 1:
                *ptr = (uint8_t)pl011_read(addr);
                break;
            case 2:
                *ptr = (uint16_t)pl011_read(addr);
                break;
            case 4:
                *(uint32_t*)ptr = pl011_read(addr);
                break;
            default:
                SCP_FATAL(()) << "Incorrect transaction size";
                break;
            }
            break;
        default:
            break;
        }
    }

    void pl011_update() { update_event.notify(); }

    void pl011_update_sysc()
    {
        uint32_t flags;
        size_t i;

        flags = s->int_level & s->int_enabled;
        if (irqmask[0] & s->int_enabled) {
            irq->write((flags & irqmask[0]) != 0);
        }
    }

    uint32_t pl011_read(uint64_t offset)
    {
        uint32_t c;
        uint64_t r;

        switch (offset >> 2) {
        case 0: /* UARTDR */
            s->flags &= ~PL011_FLAG_RXFF;
            c = s->read_fifo[s->read_pos];
            if (s->read_count > 0) {
                backend_socket.can_receive_more(1);
                s->read_count--;
                if (++s->read_pos == 16) s->read_pos = 0;
            }
            if (s->read_count == 0) {
                s->flags |= PL011_FLAG_RXFE;
            }
            if (s->read_count == s->read_trigger - 1) s->int_level &= ~PL011_INT_RX;
            s->rsr = c >> 8;
            pl011_update();
            r = c;
            break;
        case 1: /* UARTRSR */
            r = s->rsr;
            break;
        case 6: /* UARTFR */
            r = s->flags;
            break;
        case 8: /* UARTILPR */
            r = s->ilpr;
            break;
        case 9: /* UARTIBRD */
            r = s->ibrd;
            break;
        case 10: /* UARTFBRD */
            r = s->fbrd;
            break;
        case 11: /* UARTLCR_H */
            r = s->lcr;
            break;
        case 12: /* UARTCR */
            r = s->cr;
            break;
        case 13: /* UARTIFLS */
            r = s->ifl;
            break;
        case 14: /* UARTIMSC */
            r = s->int_enabled;
            break;
        case 15: /* UARTRIS */
            r = s->int_level;
            break;
        case 16: /* UARTMIS */
            r = s->int_level & s->int_enabled;
            break;
        case 18: /* UARTDMACR */
            r = s->dmacr;
            break;
        default:
            if ((offset >> 2) >= 0x3f8 && (offset >> 2) <= 0x400) {
                r = s->id[(offset - 0xfe0) >> 2];
                break;
            }
            r = 0;
            break;
        }

        return r;
    }

    void pl011_set_read_trigger()
    {
#if 0
        /* The docs say the RX interrupt is triggered when the FIFO exceeds
           the threshold.  However linux only reads the FIFO in response to an
           interrupt.  Triggering the interrupt when the FIFO is non-empty seems
           to make things work.  */
        if (s->lcr & 0x10)
            s->read_trigger = (s->ifl >> 1) & 0x1c;
        else
#endif
        s->read_trigger = 1;

        /* fixes an issue in QBox when characters are typed before linux boots */
        s->read_count = 0;
        backend_socket.can_receive_set(16);
    }

    void pl011_write(uint64_t offset, uint32_t value)
    {
        unsigned char ch;

        switch (offset >> 2) {
        case 0: /* UARTDR */
            /* ??? Check if transmitter is enabled.  */
            ch = (unsigned char)(value & 0xff);
            backend_socket.enqueue(ch);
            s->int_level |= PL011_INT_TX;
            pl011_update();
            break;
        case 1: /* UARTRSR/UARTECR */
            s->rsr = 0;
            break;
        case 6: /* UARTFR */
            /* Writes to Flag register are ignored.  */
            break;
        case 8: /* UARTUARTILPR */
            s->ilpr = value;
            break;
        case 9: /* UARTIBRD */
            s->ibrd = value;
            break;
        case 10: /* UARTFBRD */
            s->fbrd = value;
            break;
        case 11: /* UARTLCR_H */
            /* Reset the FIFO state on FIFO enable or disable */
            if ((s->lcr ^ value) & 0x10) {
                s->read_count = 0;
                s->read_pos = 0;
            }
            s->lcr = value;
            pl011_set_read_trigger();
            break;
        case 12: /* UARTCR */
            /* ??? Need to implement the enable and loopback bits.  */
            s->cr = value;
            break;
        case 13: /* UARTIFS */
            s->ifl = value;
            pl011_set_read_trigger();
            break;
        case 14: /* UARTIMSC */
            s->int_enabled = value;
            pl011_update();
            break;
        case 17: /* UARTICR */
            s->int_level &= ~value;
            pl011_update();
            break;
        case 18: /* UARTDMACR */
            s->dmacr = value;
            if (value & 3) {
            }
            break;
        default:
            break;
        }
    }

    void pl011_put_fifo(uint32_t value)
    {
        int slot;

        slot = s->read_pos + s->read_count;
        if (slot >= 16) slot -= 16;
        s->read_fifo[slot] = value;
        s->read_count++;
        s->flags &= ~PL011_FLAG_RXFE;
        if (!(s->lcr & 0x10) || s->read_count == 16) {
            s->flags |= PL011_FLAG_RXFF;
        }
        if (s->read_count == s->read_trigger) {
            s->int_level |= PL011_INT_RX;
            pl011_update();
        }
    }
    void pl011_receive(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        uint8_t* data = txn.get_data_ptr();
        for (int i = 0; i < txn.get_streaming_width(); i++) {
            pl011_put_fifo(data[i]);
        }
    }

    ~Pl011() { delete s; }
};

extern "C" void module_register();
