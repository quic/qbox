/*
 * This file is part of libqbox
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_RESET_QMP_H
#define _LIBQBOX_COMPONENTS_RESET_QMP_H

#include <ports/qemu-initiator-signal-socket.h>
#include <ports/qemu-target-signal-socket.h>
#include <ports/biflow-socket.h>
#include <device.h>
#include <qemu-instance.h>
#include <module_factory_registery.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <atomic>

#define QMP_SOCK_POLL_TIMEOUT 300
#define QMP_RECV_BUFFER_LEN   8192
class qmp : public sc_core::sc_module
{
    SCP_LOGGER();

    gs::biflow_socket_multi<qmp> qmp_socket;
    int m_sockfd = 0;

    std::string buffer = "";
    std::thread reader_thread;
    std::string socket_path;
    std::atomic_bool stop_running;

public:
    cci::cci_param<std::string> p_qmp_str;
    cci::cci_param<bool> p_monitor;

    qmp(const sc_core::sc_module_name& name, sc_core::sc_object* o): qmp(name, *(dynamic_cast<QemuInstance*>(o))) {}
    qmp(const sc_core::sc_module_name& n, QemuInstance& inst)
        : sc_core::sc_module(n)
        , p_qmp_str("qmp_str", "", "qmp options string, i.e. unix:./qmp-sock,server,wait=off")
        , qmp_socket("qmp_socket")
        , p_monitor("monitor", true, "use the HMP monitor (true, default) - or QMP (false) ")
        , stop_running{ false }
    {
        SCP_TRACE(())("qmp constructor");
        if (p_qmp_str.get_value().empty()) {
            SCP_FATAL(())("qmp options string is empty!");
        }
        if (p_qmp_str.get_value().find("unix") != std::string::npos) {
            auto first = p_qmp_str.get_value().find(":") + 1;
            auto last = p_qmp_str.get_value().find(",");
            socket_path = p_qmp_str.get_value().substr(first, last - first);
            //            unlink(socket_path.c_str());
        }
        // https://qemu-project.gitlab.io/qemu/interop/qemu-qmp-ref.html
        if (p_monitor) {
            inst.add_arg("-monitor");
        } else {
            inst.add_arg("-qmp");
        }
        inst.add_arg(p_qmp_str.get_value().c_str());

        qmp_socket.register_b_transport(this, &qmp::b_transport);
        qmp_socket.can_receive_any();
    }

    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        char* data = (char*)txn.get_data_ptr();
        int length = txn.get_data_length();

        /* collect the string in a buffer, till we see a newline, at which point, if it starts with a brace, send it as
         * is, otherwise wrap it as a human monitor command */
        buffer = buffer + std::string(data, length);
        if (data[length - 1] == '\n' || data[length - 1] == '\r') {
            if (!p_monitor) {
                buffer.erase(
                    std::remove_if(buffer.begin(), buffer.end(), [](char c) { return c == '\r' || c == '\n'; }),
                    buffer.end());
                if (buffer[0] != '{') {
                    SCP_WARN(())
                    ("Wrapping raw HMP command {} on QMP interface, consider selecting monitor mode", buffer);
                    buffer = R"("{ "execute": "human-monitor-command", "arguments": { "command-line": ")" + buffer +
                             R"(" } }")";
                }
            }
            if (m_sockfd > 0) {
                send(m_sockfd, buffer.c_str(), buffer.length(), 0);
            }
            buffer = "";
        }
        /* echo for the user */
        if (p_monitor) {
            for (int i = 0; i < txn.get_data_length(); i++) {
                qmp_socket.enqueue(txn.get_data_ptr()[i]);
            }
        }
    }

    void start_of_simulation()
    {
        reader_thread = std::thread([this]() {
            connect_to_qmp_usocket();
            struct pollfd qmp_poll;
            qmp_poll.fd = m_sockfd;
            qmp_poll.events = POLLIN;
            int ret;
            while (!stop_running) {
                ret = poll(&qmp_poll, 1, QMP_SOCK_POLL_TIMEOUT);
                if ((ret == -1 && errno == EINTR) || (ret == 0) /*timeout*/) {
                    continue;
                } else if ((ret > 0) && (qmp_poll.revents & POLLIN)) {
                    if (!qmp_recv()) break;
                } else {
                    break;
                }
            }

            if (m_sockfd > 0) {
                close(m_sockfd);
            }
            m_sockfd = 0;
        });
    }

    bool qmp_recv()
    {
        unsigned char buffer[QMP_RECV_BUFFER_LEN];
        int l = recv(m_sockfd, buffer, QMP_RECV_BUFFER_LEN, 0);
        if (l < 0) return false;
        for (int i = 0; i < l; i++) {
            qmp_socket.enqueue(buffer[i]);
        }
        return true;
    }

    void connect_to_qmp_usocket()
    {
        SCP_INFO(())("Connecting QMP socket to unix socket {}", socket_path);

        m_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_sockfd <= 0) {
            SCP_ERR(())("Unable to connect to QMP socket");
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(m_sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1) {
            SCP_ERR(())("Unable to connect to QMP socket");
        }

        if (!p_monitor) {
            std::string msg = R"({ "execute": "qmp_capabilities", "arguments": { "enable": ["oob"] } })";
            if (write(m_sockfd, msg.c_str(), msg.size()) == -1) {
                SCP_ERR(())("Can't send initialization command");
            }
        }
    }

    ~qmp()
    {
        stop_running = true;
        if (reader_thread.joinable()) {
            reader_thread.join();
        }
    }
};

extern "C" void module_register();
#endif //_LIBQBOX_COMPONENTS_QMP_H
