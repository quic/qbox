#pragma once

#include <queue>

#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

#include <libssync/async-event.h>

#define PL011_INT_TX 0x20
#define PL011_INT_RX 0x10

#define PL011_FLAG_TXFE 0x80
#define PL011_FLAG_RXFF 0x40
#define PL011_FLAG_TXFF 0x20
#define PL011_FLAG_RXFE 0x10

/* Interrupt status bits in UARTRIS, UARTMIS, UARTIMSC */
#define INT_OE (1 << 10)
#define INT_BE (1 << 9)
#define INT_PE (1 << 8)
#define INT_FE (1 << 7)
#define INT_RT (1 << 6)
#define INT_TX (1 << 5)
#define INT_RX (1 << 4)
#define INT_DSR (1 << 3)
#define INT_DCD (1 << 2)
#define INT_CTS (1 << 1)
#define INT_RI (1 << 0)
#define INT_E (INT_OE | INT_BE | INT_PE | INT_FE)
#define INT_MS (INT_RI | INT_DSR | INT_DCD | INT_CTS)

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
     const unsigned char *id;
 } PL011State;

static const uint32_t irqmask[] = {
    INT_E | INT_MS | INT_RT | INT_TX | INT_RX, /* combined IRQ */
    INT_RX,
    INT_TX,
    INT_RT,
    INT_MS,
    INT_E,
};

class CharBackend {
protected:
    void *m_opaque;
    void (*m_receive)(void *opaque, const uint8_t *buf, int size);

public:
    CharBackend()
    {
        m_opaque = NULL;
        m_receive = NULL;
    }

    virtual void write(unsigned char c) = 0;

    void register_receive(void *opaque, void (*receive)(void *opaque, const uint8_t *buf, int size))
    {
        m_opaque = opaque;
        m_receive = receive;
    }
};

#if 0
#include <ncurses.h>

class CharBackendNCurses : public CharBackend, public sc_core::sc_module {
 private:
     AsyncEvent m_event;
     std::queue<unsigned char> m_queue;

public:
    SC_HAS_PROCESS(CharBackendNCurses);

    CharBackendNCurses(sc_core::sc_module_name name)
    {
        SC_METHOD(rcv);
        sensitive << m_event;
        dont_initialize();

        pthread_t async_thread;
        if (pthread_create(&async_thread, NULL, rcv_thread, this)) {
            fprintf(stderr, "error creating thread\n");
        }
    }

    static void *rcv_thread(void *arg)
    {
        CharBackendNCurses *serial = (CharBackendNCurses *)arg;

        initscr();
        noecho();
        cbreak();
        timeout(100);

        for (;;) {
            int c = getch();
            if (c >= 0) {
                serial->m_queue.push(c);
            }
            if (!serial->m_queue.empty()) {
                serial->m_event.notify(sc_core::SC_ZERO_TIME);
            }
        }
    }

    void rcv(void)
    {
        unsigned char c;
        while (!m_queue.empty()) {
            c = m_queue.front();
            m_queue.pop();
            m_receive(m_opaque, &c, 1);
        }
    }

    void write(unsigned char c)
    {
        putchar(c);
        fflush(stdout);
    }
};
#endif

#ifndef WIN32
#include <termios.h>
#include <signal.h>
#endif

class CharBackendStdio : public CharBackend, public sc_core::sc_module {
private:
    AsyncEvent m_event;
    std::queue<unsigned char> m_queue;

public:
    SC_HAS_PROCESS(CharBackendStdio);

#ifdef WIN32
#pragma message("CharBackendStdio not yet implemented for WIN32")
#endif

    static void catch_function(int signo)
    {
        tty_reset();

#ifndef WIN32
        /* reset to default handler and raise the signal */
        signal(signo, SIG_DFL);
        raise(signo);
#endif
    }

    static void tty_reset()
    {
#ifndef WIN32
        struct termios tty;

        int fd = 0; /* stdout */

        tcgetattr(fd, &tty);

        tty.c_lflag |= ECHO | ECHONL | ICANON | IEXTEN;

        tcsetattr(fd, TCSANOW, &tty);
#endif
    }

    CharBackendStdio(sc_core::sc_module_name name)
    {
        SC_METHOD(rcv);
        sensitive << m_event;
        dont_initialize();

#ifndef WIN32
        struct termios tty;

        int fd = 0; /* stdout */

        tcgetattr(fd, &tty);

        tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);

        tcsetattr(fd, TCSANOW, &tty);

        pthread_t async_thread;
        if (pthread_create(&async_thread, NULL, rcv_thread, this)) {
            fprintf(stderr, "error creating thread\n");
        }

	signal(SIGABRT, catch_function);
	signal(SIGINT, catch_function);
	signal(SIGKILL, catch_function);
	signal(SIGSEGV, catch_function);

	atexit(tty_reset);
#endif
    }

    static void *rcv_thread(void *arg)
    {
        CharBackendStdio *serial = (CharBackendStdio *)arg;

        for (;;) {
            int c = getchar();
            if (c >= 0) {
                serial->m_queue.push(c);
            }
            if (!serial->m_queue.empty()) {
                serial->m_event.notify(sc_core::SC_ZERO_TIME);
            }
        }
    }

    void rcv(void)
    {
        unsigned char c;
        while (!m_queue.empty()) {
            c = m_queue.front();
            m_queue.pop();
            m_receive(m_opaque, &c, 1);
        }
    }

    void write(unsigned char c)
    {
        putchar(c);
        fflush(stdout);
    }
};

class Uart : public sc_core::sc_module {
public:
    PL011State *s;

    CharBackend *chr;

    tlm_utils::simple_target_socket<Uart> socket;

    sc_core::sc_vector<sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS>> irq;

    Uart(sc_core::sc_module_name name)
        : irq("irq", 6)
    {
        chr = NULL;

        socket.register_b_transport(this, &Uart::b_transport);

        s = new PL011State();

        s->read_trigger = 1;
        s->ifl = 0x12;
        s->cr = 0x300;
        s->flags = 0x90;

        static const unsigned char pl011_id_arm[8] = {
            0x11, 0x10, 0x14, 0x00, 0x0d, 0xf0, 0x05, 0xb1
        };

        s->id = pl011_id_arm;
    }

    void set_backend(CharBackend *backend)
    {
        chr = backend;
        chr->register_receive(this, pl011_receive);
    }

    void write(uint64_t offset, uint64_t value)
    {
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

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        unsigned char *ptr = trans.get_data_ptr();
        uint64_t addr = trans.get_address();

        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            pl011_write(addr, *(uint32_t *)ptr);
            break;
        case tlm::TLM_READ_COMMAND:
            *(uint32_t *)ptr = pl011_read(addr);
            break;
        default:
            break;
        }
    }

    void pl011_update()
    {
        uint32_t flags;
        size_t i;

        flags = s->int_level & s->int_enabled;
        for (i = 0; i < irq.size(); i++) {
            irq[i] = (flags & irqmask[i]) != 0;
        }
    }

    uint64_t pl011_read(uint64_t offset)
    {
        uint32_t c;
        uint64_t r;

        switch (offset >> 2) {
        case 0: /* UARTDR */
            s->flags &= ~PL011_FLAG_RXFF;
            c = s->read_fifo[s->read_pos];
            if (s->read_count > 0) {
                s->read_count--;
                if (++s->read_pos == 16)
                    s->read_pos = 0;
            }
            if (s->read_count == 0) {
                s->flags |= PL011_FLAG_RXFE;
            }
            if (s->read_count == s->read_trigger - 1)
                s->int_level &= ~ PL011_INT_RX;
            s->rsr = c >> 8;
            pl011_update();
#if 0
            qemu_chr_fe_accept_input(&s->chr);
#endif
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
#if 0
            qemu_log_mask(LOG_GUEST_ERROR,
                    "pl011_read: Bad offset 0x%x\n", (int)offset);
#endif
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
    }

    void pl011_write(uint64_t offset, uint64_t value)
    {
        unsigned char ch;

        switch (offset >> 2) {
        case 0: /* UARTDR */
            /* ??? Check if transmitter is enabled.  */
            ch = value;
            /* XXX this blocks entire thread. Rewrite to use
             * qemu_chr_fe_write and background I/O callbacks */
#if 0
            qemu_chr_fe_write_all(&s->chr, &ch, 1);
#else
            if (chr) {
                chr->write(ch);
            }
#endif
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
#if 0
                qemu_log_mask(LOG_UNIMP, "pl011: DMA not implemented\n");
#endif
            }
            break;
        default:
#if 0
            qemu_log_mask(LOG_GUEST_ERROR,
                    "pl011_write: Bad offset 0x%x\n", (int)offset);
#endif
            break;
        }
    }

    int pl011_can_receive()
    {
        int r;

        if (s->lcr & 0x10) {
            r = s->read_count < 16;
        } else {
            r = s->read_count < 1;
        }
        return r;
    }

    void pl011_put_fifo(uint32_t value)
    {
        int slot;

        slot = s->read_pos + s->read_count;
        if (slot >= 16)
            slot -= 16;
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

    static void pl011_receive(void *opaque, const uint8_t *buf, int size)
    {
        Uart *uart = (Uart *)opaque;
        uart->pl011_put_fifo(*buf);
    }

#if 0
    void pl011_event(int event)
    {
        if (event == CHR_EVENT_BREAK)
            pl011_put_fifo(0x400);
    }
#endif
};
