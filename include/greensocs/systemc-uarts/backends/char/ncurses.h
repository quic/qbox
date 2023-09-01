/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "../char-backend.h"

#include <scp/report.h>

#if 0 /* FIXME */
#include <ncurses.h>

class CharBackendNCurses : public CharBackend, public sc_core::sc_module {
private:
    AsyncEvent m_event;
    std::queue<unsigned char> m_queue;

public:
    SC_HAS_PROCESS(CharBackendNCurses);

    CharBackendNCurses(sc_core::sc_module_name name)
    {
        SCP_DEBUG(SCMOD) << "CharBackendNCurses constructor";
        SC_METHOD(rcv);
        sensitive << m_event;
        dont_initialize();

        pthread_t async_thread;
        if (pthread_create(&async_thread, NULL, rcv_thread, this)) {
            SCP_ERROR(SCMOD) << "error creating thread";
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
