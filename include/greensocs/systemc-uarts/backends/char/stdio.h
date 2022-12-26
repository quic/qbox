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

#include <unistd.h>

#ifndef WIN32
#include <greensocs/libgssync/async_event.h>
#include <queue>
#include <signal.h>
#include <termios.h>
#endif

class CharBackendStdio : public CharBackend, public sc_core::sc_module
{
private:
    gs::async_event m_event;
    std::queue<unsigned char> m_queue;
    std::mutex m_mutex;
    bool m_running = true;
    std::unique_ptr<std::thread> rcv_thread_id;
    pthread_t rcv_pthread_id = 0;

public:
    SC_HAS_PROCESS(CharBackendStdio);

#ifdef WIN32
#pragma message("CharBackendStdio not yet implemented for WIN32")
#endif
    static void catch_fn(int signo) {}
    static void catch_function(int signo) {
        tty_reset();

#ifndef WIN32
        /* reset to default handler and raise the signal */
        signal(signo, SIG_DFL);
        raise(signo);
#endif
    }

    static void tty_reset() {
#ifndef WIN32
        struct termios tty;

        int fd = 0; /* stdout */

        tcgetattr(fd, &tty);

        tty.c_lflag |= ECHO | ECHONL | ICANON | IEXTEN;

        tcsetattr(fd, TCSANOW, &tty);
#endif
    }

    CharBackendStdio(sc_core::sc_module_name name, bool read_write = true) {
        SC_METHOD(rcv);
        sensitive << m_event;
        dont_initialize();

#ifndef WIN32
        struct termios tty;

        int fd = 0; /* stdout */

        tcgetattr(fd, &tty);

        tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);

        tcsetattr(fd, TCSANOW, &tty);

        signal(SIGABRT, catch_function);
        signal(SIGINT, catch_function);
        signal(SIGKILL, catch_function);
        signal(SIGSEGV, catch_function);
        atexit(tty_reset);
#endif
        if (read_write)
            rcv_thread_id = std::make_unique<std::thread>(&CharBackendStdio::rcv_thread, this);
    }

    void* rcv_thread() {
        struct sigaction act = { 0 };
        act.sa_handler = &catch_fn;
        sigaction(SIGURG, &act, NULL);

        rcv_pthread_id = pthread_self();
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGURG);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);

        int fd = 0;
        char c;

        while (m_running) {
            int r = read(fd, &c, 1);
            if (r == 1) {
                std::lock_guard<std::mutex> lock(m_mutex);

                if (c >= 0) {
                    m_queue.push(c);
                }
                if (!m_queue.empty()) {
                    m_event.async_notify();
                }
            }
            if (r == 0) {
                break;
            }
        }

        return NULL;
    }

    void rcv(void) {
        unsigned char c;

        std::lock_guard<std::mutex> lock(m_mutex);

        while (!m_queue.empty()) {
            if (m_can_receive(m_opaque)) {
                c = m_queue.front();
                m_queue.pop();
                m_receive(m_opaque, &c, 1);
            } else {
                /* notify myself later, hopefully the queue drains */
                m_event.notify(sc_core::sc_time(1, sc_core::SC_MS));
                return;
            }
        }
    }

    void write(unsigned char c) {
        putchar(c);
        fflush(stdout);
    }

    ~CharBackendStdio() {
        m_running = false;
        pthread_kill(rcv_pthread_id, SIGURG);
        rcv_thread_id->join();
    }
};
