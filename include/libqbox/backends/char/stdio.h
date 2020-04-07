/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps
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

#include "../char-backend.h"

#ifndef WIN32
#include <termios.h>
#include <signal.h>
#include <sys/epoll.h>
#endif

class CharBackendStdio : public CharBackend, public sc_core::sc_module {
private:
    AsyncEvent m_event;
    std::queue<unsigned char> m_queue;
    std::mutex m_mutex;

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

        signal(SIGABRT, catch_function);
        signal(SIGINT, catch_function);
        signal(SIGKILL, catch_function);
        signal(SIGSEGV, catch_function);

        atexit(tty_reset);
#endif

        new std::thread(&CharBackendStdio::rcv_thread, this);
    }

    void *rcv_thread()
    {
#ifndef _WIN32
        int epollfd;
        struct epoll_event ev, events[1];

        epollfd = epoll_create1(0);
        if (epollfd == -1) {
            perror("epoll_fd");
            return NULL;
        }

        ev.events = EPOLLIN;
        ev.data.fd = STDIN_FILENO;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
            perror("epoll_ctl");
            return NULL;
        }
#endif

        for (;;) {
#ifndef _WIN32
            int nfds = epoll_wait(epollfd, events, 1, -1);
            if (nfds == -1) {
                perror("epoll_wait");
                break;
            }
#endif

            int c = getchar();

            std::lock_guard<std::mutex> lock(m_mutex);

            if (c >= 0) {
                m_queue.push(c);
            }
            if (!m_queue.empty()) {
                m_event.async_notify();
            }
        }

        return NULL;
    }

    void rcv(void)
    {
        unsigned char c;

        std::lock_guard<std::mutex> lock(m_mutex);

        while (!m_queue.empty()) {
            if (m_can_receive(m_opaque)) {
                c = m_queue.front();
                m_queue.pop();
                m_receive(m_opaque, &c, 1);
            }
            else {
                /* notify myself later, hopefully the queue drains */
                m_event.notify(1, sc_core::SC_MS);
                return;
            }
        }
    }

    void write(unsigned char c)
    {
        putchar(c);
        fflush(stdout);
    }
};
