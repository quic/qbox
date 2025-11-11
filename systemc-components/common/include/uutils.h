/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_UUTILS
#define _GREENSOCS_BASE_COMPONENTS_UUTILS

#include <cassert>
#include <iostream>
#include <thread>
#include <map>
#include <utility>
#include <functional>
#include <vector>
#include <atomic>
#include <systemc>
#include <unordered_map>
#include <scp/report.h>
#include <mutex>
#include <cstdlib>
#include <csignal>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#else
#include <windows.h>
#endif

namespace gs {

enum class Handler_CB {
    EXIT,       //  run signal_handler then exit
    PASS,       // run signal_handler
    FORCE_EXIT, // just exit
    IGN,        // ignore
    DFL,        // default
};

class SigHandler : public sc_core::sc_prim_channel
{
public:
    static SigHandler& get();

    /**
     * @brief Add a SIGINT handler with the specified callback type.
     * @param s_cb Handler callback type.
     */
    void add_sigint_handler(Handler_CB s_cb);

    template <typename CONT_TYPE>
    void register_cb(const std::string& name, std::unordered_map<std::string, std::function<CONT_TYPE>>& container,
                     const std::function<CONT_TYPE>& cb, const std::string& type_of_cb);

    template <typename CONT_TYPE>
    void deregister_cb(const std::string& name, std::unordered_map<std::string, std::function<CONT_TYPE>>& container,
                       const std::string& type_of_cb);

    void register_on_exit_cb(const std::string& name, const std::function<void()>& cb);

    void deregister_on_exit_cb(const std::string& name);

    void register_handler(const std::string& name, const std::function<void(int)>& handler);

    void deregister_handler(const std::string& name);

    ~SigHandler();

    void end_of_simulation();

    void set_sig_num(int val);

#ifdef _WIN32
    void console_signal_dispatch(int sig_num);
    void register_all_signal_handlers();
    void register_signal_handler(int sig);
#else
    static void pass_sig_handler(int sig);

    static void force_exit_sig_handler(int sig);

    void add_to_block_set(int signum);

    void reset_block_set();

    void block_signal_set();

    /**
     * @brief Add a SIGCHLD handler with the specified callback type.
     * @param s_cb Handler callback type.
     */
    void add_sigchld_handler(Handler_CB s_cb);

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

    int get_write_sock_end();

    void mark_error_signal(int signum, std::string error_msg);

#endif // _WIN32
private:
    void update();

    SigHandler();

    SigHandler(const SigHandler&) = delete;
    SigHandler& operator()(const SigHandler&) = delete;

    void _start_pass_signal_handler();
    void _update_sig_cb(int signum, Handler_CB s_cb);

    void _update_all_sigs_cb();

    void _change_sig_cbs_to_dfl();

    void add_sig_handler(int signum, Handler_CB s_cb);

private:
    std::map<int, Handler_CB> m_signals;
    std::map<int, std::string> error_signals;
    std::unordered_map<std::string, std::function<void(int)>> handlers;
    std::unordered_map<std::string, std::function<void()>> exit_handlers;
    std::mutex cb_mutex;
    std::mutex thread_start_mutex;
    std::mutex signals_mutex;
    std::thread pass_handler;
    std::atomic_bool stop_running;
    std::atomic_bool is_pass_handler_requested;
    std::atomic_int32_t sig_num;

#ifdef _WIN32
    HANDLE m_console_ctrl_event;
#else
    sigset_t m_sigs_to_block;
    struct pollfd self_pipe_monitor;

    int self_sockpair_fd[2];
    struct sigaction exit_act;
    struct sigaction pass_act;
    struct sigaction force_exit_act;
    struct sigaction ign_act;
    struct sigaction dfl_act;
#endif // _WIN32
};

#ifndef _WIN32
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
    void init_peer_conn_checker();

private:
    std::atomic_bool stop_running;
    pid_t m_ppid = -1;
    std::thread child_waiter;
    std::thread parent_alive_checker;
    int m_sock_pair_fds[2];
};
#endif //
} // namespace gs

#endif // _GREENSOCS_BASE_COMPONENTS_UUTILS