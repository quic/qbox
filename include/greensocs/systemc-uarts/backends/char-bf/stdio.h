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

#ifndef _GS_UART_BACKEND_STDIO_H_
#define _GS_UART_BACKEND_STDIO_H_

#include <systemc>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>

#include <unistd.h>

#include <greensocs/libgssync/async_event.h>
#include <greensocs/gsutils/uutils.h>
#include <greensocs/gsutils/ports/biflow-socket.h>
#include <greensocs/gsutils/module_factory_registery.h>
#include <queue>
#include <signal.h>
#include <termios.h>

class CharBFBackendStdio : public sc_core::sc_module
{
private:
    bool m_running = true;
    std::thread rcv_thread_id;
    pthread_t rcv_pthread_id = 0;

    SCP_LOGGER();

public:
    gs::biflow_socket<CharBFBackendStdio> socket;

#ifdef WIN32
#pragma message("CharBFBackendStdio not yet implemented for WIN32")
#endif
    static void catch_fn(int signo) {}

    static void tty_reset()
    {
        struct termios tty;

        int fd = 0; /* stdout */

        tcgetattr(fd, &tty);

        tty.c_lflag |= ECHO | ECHONL | ICANON | IEXTEN;

        tcsetattr(fd, TCSANOW, &tty);
    }

    CharBFBackendStdio(sc_core::sc_module_name name, bool read_write = true)
        : sc_core::sc_module(name), socket("biflow_socket")
    {
        SCP_TRACE(()) << "constructor";

        struct termios tty;

        int fd = 0; /* stdout */

        tcgetattr(fd, &tty);

        tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);

        tcsetattr(fd, TCSANOW, &tty);

        atexit(tty_reset);

        gs::SigHandler::get().register_on_exit_cb(tty_reset);
        gs::SigHandler::get().add_sig_handler(SIGINT, gs::SigHandler::Handler_CB::PASS);
        gs::SigHandler::get().register_handler([&](int signo) {
            if (signo == SIGINT) {
                char ch = '\x03';
                socket.enqueue(ch);
            }
        });
        if (read_write) rcv_thread_id = std::thread(&CharBFBackendStdio::rcv_thread, this);

        socket.register_b_transport(this, &CharBFBackendStdio::writefn);
    }

    void end_of_elaboration() { socket.can_receive_any(); }

    void* rcv_thread()
    {
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
                socket.enqueue(c);
            }
            if (r == 0) {
                break;
            }
        }

        return NULL;
    }

    void writefn(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        uint8_t* data = txn.get_data_ptr();
        for (int i = 0; i < txn.get_data_length(); i++) {
            putchar(data[i]);
        }
        fflush(stdout);
    }

    ~CharBFBackendStdio()
    {
        m_running = false;
        if (rcv_pthread_id) pthread_kill(rcv_pthread_id, SIGURG);
        if (rcv_thread_id.joinable()) rcv_thread_id.join();
    }
};
GSC_MODULE_REGISTER(CharBFBackendStdio, bool);
#endif
