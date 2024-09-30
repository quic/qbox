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

class qmp : public sc_core::sc_module
{
    SCP_LOGGER();

    gs::biflow_socket_multi<qmp> qmp_socket;
    int m_sockfd = 0;

    std::string buffer = "";
    std::thread reader_thread;
    std::string socket_path;

public:
    cci::cci_param<std::string> p_qmp_str;

    qmp(const sc_core::sc_module_name& name, sc_core::sc_object* o): qmp(name, *(dynamic_cast<QemuInstance*>(o))) {}
    qmp(const sc_core::sc_module_name& n, QemuInstance& inst)
        : sc_core::sc_module(n)
        , p_qmp_str("qmp_str", "", "qmp options string, i.e. unix:./qmp-sock,server,wait=off")
        , qmp_socket("qmp_socket")
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
        inst.add_arg("-qmp");
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
            buffer.erase(std::remove_if(buffer.begin(), buffer.end(), [](char c) { return c == '\r' || c == '\n'; }),
                         buffer.end());
            std::string msg;
            if (buffer[0] != '{') {
                msg = R"("{ "execute": "human-monitor-command", "arguments": { "command-line": ")" + buffer +
                      R"(" } }")";
            } else {
                msg = buffer;
            }
            if (m_sockfd > 0) {
                send(m_sockfd, msg.c_str(), msg.length(), 0);
            }
            buffer = "";
        }
        /* echo for the user */
        for (int i = 0; i < txn.get_data_length(); i++) {
            qmp_socket.enqueue(txn.get_data_ptr()[i]);
        }
    }

    void start_of_simulation()
    {
        reader_thread = std::thread([&]() {
            unsigned char buffer[1024];

            sleep(1);
            connect_to_qmp_usocket();

            while (m_sockfd > 0) {
                int l = recv(m_sockfd, buffer, 1024, 0);
                if (l < 0) break;
                for (int i = 0; i < l; i++) {
                    qmp_socket.enqueue(buffer[i]);
                }
            }

            if (m_sockfd > 0) {
                close(m_sockfd);
            }
            m_sockfd = 0;
        });
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

        std::string msg = R"({ "execute": "qmp_capabilities", "arguments": { "enable": ["oob"] } })";
        if (write(m_sockfd, msg.c_str(), msg.size()) == -1) {
            SCP_ERR(())("Can't send initialization command");
        }
    }
};

extern "C" void module_register();
#endif //_LIBQBOX_COMPONENTS_QMP_H
