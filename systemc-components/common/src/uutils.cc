/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <uutils.h>

gs::SigHandler& gs::SigHandler::get()
{
    static SigHandler sh;
    return sh;
}

void gs::SigHandler::pass_sig_handler(int sig)
{
    SigHandler::get().set_sig_num(sig);
    char ch[1] = { 's' };
    ssize_t bytes_written = ::write(SigHandler::get().get_write_sock_end(), &ch, 1);
    if (bytes_written < 0) _Exit(EXIT_FAILURE);
}

void gs::SigHandler::force_exit_sig_handler(int sig)
{
    _Exit(EXIT_SUCCESS); // FIXME: should the exit status be EXIT_FAILURE?
}

void gs::SigHandler::add_sig_handler(int signum, Handler_CB s_cb = Handler_CB::EXIT)
{
    if (m_signals.find(signum) != m_signals.end())
        m_signals.at(signum) = s_cb;
    else {
        auto ret = m_signals.insert(std::make_pair(signum, s_cb));
        if (!ret.second) {
            std::cerr << "Failed to insert signal: " << signum << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    _update_sig_cb(signum, s_cb);
}

void gs::SigHandler::register_on_exit_cb(std::function<void()> cb) { exit_handlers.push_back(cb); }

void gs::SigHandler::add_to_block_set(int signum) { sigaddset(&m_sigs_to_block, signum); }

void gs::SigHandler::reset_block_set() { sigemptyset(&m_sigs_to_block); }

void gs::SigHandler::block_signal_set() { sigprocmask(SIG_BLOCK, &m_sigs_to_block, NULL); }

void gs::SigHandler::set_nosig_chld_stop() { exit_act.sa_flags = SA_NOCLDSTOP; }

void gs::SigHandler::block_curr_handled_signals()
{
    reset_block_set();
    for (auto sig_cb_pair : m_signals) add_to_block_set(sig_cb_pair.first);
    block_signal_set();
}

void gs::SigHandler::register_handler(std::function<void(int)> handler)
{
    handlers.push_back(handler);
    _start_pass_signal_handler();
}

int gs::SigHandler::get_write_sock_end()
{
    int flags = fcntl(self_sockpair_fd[1],
                      F_GETFL); // fcntl is async signal safe according to
                                // https://man7.org/linux/man-pages/man7/signal-safety.7.html
    if (flags == -1) {
        perror("fcntl F_GETFL");
        _Exit(EXIT_FAILURE);
    }
    if (!(flags & O_NONBLOCK)) {
        flags |= O_NONBLOCK;
        if (fcntl(self_sockpair_fd[1], F_SETFL, flags) == -1) {
            perror("fcntl F_SETFL");
            _Exit(EXIT_FAILURE);
        }
    }
    return self_sockpair_fd[1];
}

void gs::SigHandler::set_sig_num(int val) { sig_num = val; }

void gs::SigHandler::mark_error_signal(int signum, std::string error_msg)
{
    if (error_signals.find(signum) != error_signals.end())
        error_signals.at(signum) = error_msg;
    else {
        auto ret = error_signals.insert(std::make_pair(signum, error_msg));
        if (!ret.second) {
            std::cerr << "Failed to insert error signal: " << signum << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

gs::SigHandler::~SigHandler()
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        stop_running = true; // this should cause the pass_handler thread to exit next time it checks the flag.

        exit_handlers.clear();
        handlers.clear();
    }
    if (pass_handler.joinable()) pass_handler.join();
    _change_sig_cbs_to_dfl();
    close(self_sockpair_fd[0]);
    close(self_sockpair_fd[1]);
}

void gs::SigHandler::_start_pass_signal_handler()
{
    if (is_pass_handler_requested) return;
    pass_handler = std::thread([this]() {
        self_pipe_monitor.fd = self_sockpair_fd[0];
        self_pipe_monitor.events = POLLIN;
        int ret;
        while (!stop_running) {
            ret = poll(&self_pipe_monitor, 1, 500);
            if ((ret == -1 && errno == EINTR) || (ret == 0) /*timeout*/)
                continue;
            else if (self_pipe_monitor.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                break;
            } else if (self_pipe_monitor.revents & POLLIN) {
                std::unique_lock<std::mutex> lock(mutex);
                if (stop_running) break;
                char ch[1];
                if (read(self_pipe_monitor.fd, ch, 1) == -1) {
                    perror("pipe read");
                    exit(EXIT_FAILURE);
                }
                if (m_signals[sig_num] == Handler_CB::EXIT) {
                    stop_running = true;
                    if (sc_core::sc_get_status() < sc_core::SC_RUNNING) {
                        _Exit(EXIT_SUCCESS); // FIXME: should the exit status be EXIT_FAILURE?
                    }
                    if ((sc_core::sc_get_status() != sc_core::SC_STOPPED) &&
                        (sc_core::sc_get_status() !=
                         sc_core::SC_END_OF_SIMULATION)) { // don't call if sc_stop was already called, because
                                                           // destructors of other classes may have been already
                                                           // called and the order of destruction of objects may
                                                           // cause the exit callback functions to be called on
                                                           // non-existing objects
                        for (auto on_exit_cb : exit_handlers) on_exit_cb();
                        exit_handlers.clear();
                        handlers.clear();
                        async_request_update();
                    }
                    if (error_signals.find(sig_num) != error_signals.end()) {
                        std::cerr << "\nFatal error: " << error_signals[sig_num] << std::endl;
                        _Exit(EXIT_FAILURE);
                    }
                } else {
                    for (auto handler : handlers) handler(sig_num);
                }
            } else {
                exit(EXIT_FAILURE);
            }
        }
    });
    is_pass_handler_requested = true;
}

void gs::SigHandler::update()
{
    if (stop_running) sc_core::sc_stop();
}

gs::SigHandler::SigHandler()
    : sc_core::sc_prim_channel("SigHandler")
    , m_signals{ { SIGINT, Handler_CB::EXIT },  { SIGTERM, Handler_CB::EXIT }, { SIGQUIT, Handler_CB::EXIT },
                 { SIGSEGV, Handler_CB::EXIT }, { SIGABRT, Handler_CB::EXIT }, { SIGBUS, Handler_CB::EXIT } }
    , error_signals{ { SIGSEGV, "segmentation fault (SIGSEGV)!" },
                     { SIGBUS, "bus error (SIGBUS)!" },
                     { SIGABRT, "abnormal termination (SIGABRT)!" } }
    , is_pass_handler_requested{ false }
    , stop_running{ false }
{
    memset(&dfl_act, 0, sizeof(dfl_act));
    memset(&ign_act, 0, sizeof(ign_act));
    memset(&pass_act, 0, sizeof(pass_act));
    memset(&exit_act, 0, sizeof(exit_act));
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, self_sockpair_fd) == -1) {
        perror("SigHnadler socketpair");
        std::exit(EXIT_FAILURE);
    }
    reset_block_set();
    _update_all_sigs_cb();
}

void gs::SigHandler::_update_sig_cb(int signum, Handler_CB s_cb)
{
    switch (s_cb) {
    case Handler_CB::EXIT:
        exit_act.sa_handler = SigHandler::pass_sig_handler;
        sigaction(signum, &exit_act, NULL);
        _start_pass_signal_handler();
        break;
    case Handler_CB::PASS:
        pass_act.sa_handler = SigHandler::pass_sig_handler;
        sigaction(signum, &pass_act, NULL);
        break;
    case Handler_CB::FORCE_EXIT:
        force_exit_act.sa_handler = SigHandler::force_exit_sig_handler;
        sigaction(signum, &force_exit_act, NULL);
        break;
    case Handler_CB::IGN:
        ign_act.sa_handler = SIG_IGN;
        sigaction(signum, &ign_act, NULL);
        break;
    case Handler_CB::DFL:
        dfl_act.sa_handler = SIG_DFL;
        sigaction(signum, &dfl_act, NULL);
        break;
    }
}

void gs::SigHandler::_update_all_sigs_cb()
{
    for (auto sig_cb_pair : m_signals) {
        _update_sig_cb(sig_cb_pair.first, sig_cb_pair.second);
    }
}

void gs::SigHandler::_change_sig_cbs_to_dfl()
{
    for (auto sig_cb_pair : m_signals) {
        if (sig_cb_pair.second != Handler_CB::DFL) _update_sig_cb(sig_cb_pair.first, Handler_CB::DFL);
    }
}

gs::ProcAliveHandler::ProcAliveHandler(): stop_running(false), m_is_ppid_set{ false }, m_is_parent_setup_called{ false }
{
}

gs::ProcAliveHandler::~ProcAliveHandler()
{
    close(m_sock_pair_fds[0]);
    close(m_sock_pair_fds[1]);
    stop_running = true;
    if (parent_alive_checker.joinable()) parent_alive_checker.join();
    if (child_waiter.joinable()) child_waiter.join();
}

void gs::ProcAliveHandler::wait_child(pid_t cpid)
{
    child_waiter = std::thread(
        [](pid_t cpid) {
            pid_t m_wpid;
            int wstatus;
            for (;;) {
                m_wpid = waitpid(cpid, &wstatus, 0);
                if (m_wpid == -1) {
                    exit(EXIT_FAILURE);
                }
                if (WIFEXITED(wstatus)) {
                    std::cerr << "process (" << m_wpid << ") exited, status= " << WEXITSTATUS(wstatus) << std::endl;
                    break;
                }
            } // end for
        },
        cpid); // end lambda
} // wait_child()

void gs::ProcAliveHandler::recv_sockpair_fds_from_remote(int fd0, int fd1)
{
    m_sock_pair_fds[0] = fd0;
    m_sock_pair_fds[1] = fd1;
}

int gs::ProcAliveHandler::get_sockpair_fd0() const { return m_sock_pair_fds[0]; }

int gs::ProcAliveHandler::get_sockpair_fd1() const { return m_sock_pair_fds[1]; }

pid_t gs::ProcAliveHandler::get_ppid() const
{
    if (m_is_ppid_set)
        return m_ppid;
    else
        return -1;
}

void gs::ProcAliveHandler::init_peer_conn_checker()
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_sock_pair_fds) == -1) {
        perror("ProcAliveHandler socketpair");
        std::exit(EXIT_FAILURE);
    }
}

void gs::ProcAliveHandler::setup_parent_conn_checker()
{
    m_is_parent_setup_called = true;
    close(m_sock_pair_fds[1]);
}

/**
 * same thread parent connected checker
 */
void gs::ProcAliveHandler::check_parent_conn_sth(std::function<void()> on_parent_exit)
{
    _set_ppid();
    close(m_sock_pair_fds[0]);
    struct pollfd parent_connected_monitor;
    parent_connected_monitor.fd = m_sock_pair_fds[1];
    parent_connected_monitor.events = POLLIN;
    int ret = 0;
    while (!stop_running) {
        ret = poll(&parent_connected_monitor, 1, 500);
        if ((ret == -1 && errno == EINTR) || (ret == 0) /*timeout*/)
            continue;
        else if (parent_connected_monitor.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            on_parent_exit();
            exit(EXIT_SUCCESS);
        } else {
            perror("poll");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * new thread parent connected checker
 */
void gs::ProcAliveHandler::check_parent_conn_nth(std::function<void()> on_parent_exit)
{
    parent_alive_checker = std::thread(&ProcAliveHandler::check_parent_conn_sth, this, on_parent_exit);
}

void gs::ProcAliveHandler::_set_ppid()
{
    m_ppid = getppid();
    m_is_ppid_set = true;
}
