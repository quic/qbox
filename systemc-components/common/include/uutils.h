/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_UUTILS
#define _GREENSOCS_BASE_COMPONENTS_UUTILS

#ifndef WIN32

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <csignal>
#include <signal.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#include <cassert>
#include <iostream>
#include <thread>
#include <map>
#include <utility>
#include <functional>
#include <vector>
#include <atomic>
#include <systemc>

#include <scp/report.h>

namespace gs {

class SigHandler : public sc_core::sc_prim_channel
{
public:
    static SigHandler& get();

    enum class Handler_CB {
        EXIT,       //  run signal_handler then exit
        PASS,       // run signal_handler
        FORCE_EXIT, // just exit
        IGN,        // ignore
        DFL,        // default
    };

    static void pass_sig_handler(int sig);

    static void force_exit_sig_handler(int sig);

    void add_sig_handler(int signum, Handler_CB s_cb);

    void register_on_exit_cb(std::function<void()> cb);

    void add_to_block_set(int signum);

    void reset_block_set();

    void block_signal_set();

    /**
     *  SIGCHLD will not be sent when child stopped or continued,
     *  it will be sent only when killed.
     */
    void set_nosig_chld_stop();

    /**
     * Block all the signals previously added to the class instance
     * by default in the class constructor or by calling add_sig_handler().
     * All signals added to the block list previously using add_to_block_set()
     * will be removed first.
     */
    void block_curr_handled_signals();

    void register_handler(std::function<void(int)> handler);

    int get_write_sock_end();

    void set_sig_num(int val);

    void mark_error_signal(int signum, std::string error_msg);

    ~SigHandler();

private:
    void _start_pass_signal_handler();

    void update();

    SigHandler();

    SigHandler(const SigHandler&) = delete;
    SigHandler& operator()(const SigHandler&) = delete;

    void _update_sig_cb(int signum, Handler_CB s_cb);

    void _update_all_sigs_cb();

    void _change_pass_sig_cbs_to_force_exit();

private:
    std::map<int, Handler_CB> m_signals;
    std::map<int, std::string> error_signals;
    sigset_t m_sigs_to_block;
    struct pollfd self_pipe_monitor;
    std::thread pass_handler;
    std::vector<std::function<void(int)>> handlers;
    std::vector<std::function<void()>> exit_handlers;
    bool is_pass_handler_requested;
    std::atomic_bool stop_running;
    std::atomic_int32_t sig_num;
    int self_sockpair_fd[2];
    struct sigaction exit_act;
    struct sigaction pass_act;
    struct sigaction force_exit_act;
    struct sigaction ign_act;
    struct sigaction dfl_act;
};

class ProcAliveHandler
{
public:
    ProcAliveHandler();

    ~ProcAliveHandler();

    void wait_child(pid_t cpid);

    void recv_sockpair_fds_from_remote(int fd0, int fd1);

    int get_sockpair_fd0() const;

    int get_sockpair_fd1() const;

    pid_t get_ppid() const;

    void init_peer_conn_checker();

    void setup_parent_conn_checker();

    /**
     * same thread parent connected checker
     */
    void check_parent_conn_sth(std::function<void()> on_parent_exit);

    /**
     * new thread parent connected checker
     */
    void check_parent_conn_nth(std::function<void()> on_parent_exit);

private:
    void _set_ppid();

private:
    std::atomic_bool stop_running;
    pid_t m_ppid;
    std::thread child_waiter;
    std::thread parent_alive_checker;
    int m_sock_pair_fds[2];
    bool m_is_ppid_set;
    bool m_is_parent_setup_called;
};

} // namespace gs

#endif // WIN32

#endif // _GREENSOCS_BASE_COMPONENTS_UUTILS