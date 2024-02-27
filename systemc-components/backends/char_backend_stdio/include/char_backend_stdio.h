/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GS_UART_BACKEND_STDIO_H_
#define _GS_UART_BACKEND_STDIO_H_

#include <systemc>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>

#include <unistd.h>

#include <async_event.h>
#include <uutils.h>
#include <ports/biflow-socket.h>
#include <module_factory_registery.h>
#include <queue>
#include <signal.h>
#include <termios.h>
#include <poll.h>

class char_backend_stdio : public sc_core::sc_module
{
protected:
    cci::cci_param<bool> p_read_write;

private:
    bool m_running = true;
    std::thread rcv_thread_id;
    pthread_t rcv_pthread_id = 0;
    std::atomic<bool> c_flag;
    SCP_LOGGER();

public:
    gs::biflow_socket<char_backend_stdio> socket;

#ifdef WIN32
#pragma message("CharBackendStdio not yet implemented for WIN32")
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

    char_backend_stdio(sc_core::sc_module_name name)
        : sc_core::sc_module(name)
        , p_read_write("read_write", true, "read_write if true start rcv_thread")
        , socket("biflow_socket")
        , c_flag(false)
    {
        SCP_TRACE(()) << "CharBackendStdio constructor";

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
                c_flag = true;
            }
        });
        if (p_read_write) rcv_thread_id = std::thread(&char_backend_stdio::rcv_thread, this);

        socket.register_b_transport(this, &char_backend_stdio::writefn);
    }

    void end_of_elaboration() { socket.can_receive_any(); }

    void enqueue(char c) { socket.enqueue(c); }
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

        struct pollfd self_pipe_monitor;
        self_pipe_monitor.fd = fd;
        self_pipe_monitor.events = POLLIN;

        while (m_running) {
            int ret = poll(&self_pipe_monitor, 1, 300);
            if ((ret == -1 && errno == EINTR) || (ret == 0) /*timeout*/) {
                if (c_flag) {
                    c_flag = false;
                    enqueue('\x03');
                }
                continue;
            } else if (self_pipe_monitor.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                break;
            } else if (self_pipe_monitor.revents & POLLIN) {
                char c;
                int r = read(fd, &c, 1);
                if (r == 1) {
                    enqueue(c);
                }
                if (r == 0) {
                    break;
                }
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

    ~char_backend_stdio()
    {
        m_running = false;
        if (rcv_pthread_id) pthread_kill(rcv_pthread_id, SIGURG);
        if (rcv_thread_id.joinable()) rcv_thread_id.join();
    }
};
extern "C" void module_register();
#endif
