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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
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
