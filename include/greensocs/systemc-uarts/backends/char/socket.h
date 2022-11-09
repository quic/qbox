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

#ifndef WIN32
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <netdb.h>

#include <scp/report.h>

#include <greensocs/libgssync/async_event.h>
#endif

class CharBackendSocket : public CharBackend, public sc_core::sc_module {
private:
    gs::async_event m_event;
    std::queue<unsigned char> m_queue;
    std::mutex m_mutex;

public:
    SC_HAS_PROCESS(CharBackendSocket);

#ifdef WIN32
#pragma message("CharBackendSocket not yet implemented for WIN32")
#endif

    std::string type;
    std::string address;
    bool m_server;
    int m_srv_socket;
    bool m_nowait;
    int m_socket = -1;
    uint8_t m_buf[256];

    void start_of_simulation() {
        if (type == "tcp") {
            std::string ip = "127.0.0.1";
            std::string port = "4001";

            size_t count = std::count(address.begin(), address.end(), ':');
            if (count == 1) {
                size_t first = address.find_first_of(':');
                ip = address.substr(0, first);
                port = address.substr(first + 1);
            } else {
                // error
                SCP_ERR(SCMOD) << "malformed address, expecting IP:PORT (e.g 127.0.0.1:4001)";
                return;
            }

            SCP_DEBUG(SCMOD) << "IP: " << ip << ", PORT: " << port;

            if (m_server) {
                setup_tcp_server(ip, port);
            } else {
                setup_tcp_client(ip, port);
            }
        } else if (type == "udp") {
            SCP_ERR(SCMOD) << "udp sockets are not available in this version";
            return;
        } else if (type == "unix") {
            SCP_ERR(SCMOD) << "unix sockets are not available in this version";
            return;
        } else {
            SCP_ERR(SCMOD) << "bad value for socket type";
            return;
        }

        new std::thread(&CharBackendSocket::rcv_thread, this);
    }

    CharBackendSocket(sc_core::sc_module_name name, std::string type, std::string address,
                      bool server = true, bool nowait = true)
        : type(type), address(address), m_server(server), m_nowait(nowait) {
        SC_METHOD(rcv);
        sensitive << m_event;
        dont_initialize();
    }

    void* rcv_thread() {
        if (!m_nowait && ((!m_server && m_socket < 0) || (m_server && m_srv_socket < 0))) {
            return NULL;
        }

        for (;;) {
            if (m_nowait && m_server && m_socket < 0) {
                socklen_t addr_len = sizeof(struct sockaddr_in);
                struct sockaddr_in client_addr;
                int flag;

                m_socket = ::accept(m_srv_socket, (struct sockaddr*)&client_addr, &addr_len);
                if (m_socket < 0) {
                    continue;
                }

#if 0
                flag = 1;
                if(::ioctl(m_socket, FIONBIO, (char *)&flag) < 0) {
                    SCP_ERR(SCMOD) << "setting socket in blocking mode failed: " << std::strerror(errno);
                    continue;
                }
#endif

                flag = 1;
                if (::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag))) {
                    SCP_WARN(SCMOD)
                        << "setting up TCP_NODELAY option failed: " << std::strerror(errno);
                }

                char str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);
                int cport = ntohs(client_addr.sin_port);

                SCP_DEBUG(SCMOD) << "incoming connection from  " << str << ":" << cport;
            } else if (m_nowait && !m_server && m_socket < 0) {
                // TODO
                continue;
            }

            int ret = ::read(m_socket, &m_buf[0], 256);
            if (ret < 0 && errno != EAGAIN && errno != EINTR) {
                SCP_ERR(SCMOD) << "read failed: " << std::strerror(errno);
                abort();
            } else if (ret > 0) {
                for (int i = 0; i < ret; i++) {
                    unsigned char c = m_buf[i];
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_queue.push(c);
                    m_event.async_notify();
                }
            }
        }
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
                /* notify myself, hopefully the queue drains */
                m_event.notify();
                return;
            }
        }
    }

    void write(unsigned char c) {
        if (!m_nowait && ((!m_server && m_socket < 0) || (m_server && m_srv_socket < 0))) {
            return;
        }

        while (m_nowait && m_socket < 0) {
            sc_core::wait(1, sc_core::SC_MS);
        }

#if 0
        SCP_INFO(SCMOD) << "Got " << int(c) << "(" << c << ")";
#endif

        ::write(m_socket, &c, 1);
    }

    void setup_tcp_server(std::string ip, std::string port) {
        int ret;
        struct sockaddr_in addr_in;
        int iport;
        int flag;

        SCP_DEBUG(SCMOD) << "setting up TCP server on " << ip << ":" << port;

        socklen_t addr_len = sizeof(struct sockaddr_in);

        m_srv_socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_srv_socket < 0) {
            SCP_ERR(SCMOD) << "socket failed: " << std::strerror(errno);
            return;
        }

        flag = 1;
        ::setsockopt(m_srv_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

        iport = std::stoi(port);

        ::memset((char*)&addr_in, 0, sizeof(addr_in));
        addr_in.sin_family = AF_INET;
        addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
        addr_in.sin_port = htons(iport);

        ret = ::bind(m_srv_socket, (struct sockaddr*)&addr_in, sizeof(addr_in));
        if (ret < 0) {
            ::close(m_srv_socket);
            m_srv_socket = -1;
            SCP_ERR(SCMOD) << "bind failed: " << std::strerror(errno);
            return;
        }

        ret = listen(m_srv_socket, 1);
        if (ret < 0) {
            ::close(m_srv_socket);
            m_srv_socket = -1;
            SCP_ERR(SCMOD) << "listen failed: " << std::strerror(errno);
            return;
        }

        if (m_nowait) {
#if 0
            flag = 1;
            if(::ioctl(m_srv_socket, FIONBIO, (char *)&flag) < 0) {
                SCP_ERR(SCMOD) << "setting socket in non-blocking mode failed: " << std::strerror(errno);
                return;
            }
#endif

            // accept() will be done later in the SC_THREAD
        } else {
            SCP_INFO(SCMOD) << "waiting for a connection on " << ip << ":" << port;

            struct sockaddr_in client_addr;

            m_socket = ::accept(m_srv_socket, (struct sockaddr*)&client_addr, &addr_len);
            if (m_socket < 0) {
                ::close(m_srv_socket);
                m_srv_socket = -1;
                SCP_ERR(SCMOD) << "accept failed: " << std::strerror(errno);
            }

#if 0
            flag = 1;
            if(::ioctl(m_socket, FIONBIO, (char *)&flag) < 0) {
                SCP_ERR(SCMOD) << "setting socket in blocking mode failed: " << std::strerror(errno);
                return;
            }
#endif

            flag = 1;
            if (::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag))) {
                SCP_WARN(SCMOD) << "setting up TCP_NODELAY option failed: " << std::strerror(errno);
            }

            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);
            int cport = ntohs(client_addr.sin_port);

            SCP_DEBUG(SCMOD) << "incoming connection from  " << str << ":" << cport;
        }
    }

    void setup_tcp_client(std::string ip, std::string port) {
        int flag = 1;
        int status;
        struct addrinfo hints;
        struct addrinfo* servinfo;

        SCP_INFO(SCMOD) << "setting up TCP client connection to " << ip << ":" << port;

        m_socket = ::socket(AF_INET, SOCK_STREAM, 0);

        if (m_socket == -1) {
            SCP_ERR(SCMOD) << "socket failed: " << std::strerror(errno);
            return;
        }

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        status = getaddrinfo(ip.c_str(), port.c_str(), &hints, &servinfo);
        if (status == -1) {
            SCP_ERR(SCMOD) << "getaddrinfo failed: " << std::strerror(errno);
            goto close_sock;
        }

        if (m_nowait) {
            flag = 1;
            if (::ioctl(m_socket, FIONBIO, (char*)&flag) < 0) {
                SCP_ERR(SCMOD) << "setting socket in non-blocking mode failed: "
                               << std::strerror(errno);
                goto close_sock;
            }

            // connect() will be done later in the SC_THREAD
        } else {
            if (::connect(m_socket, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
                SCP_ERR(SCMOD) << "connect failed: " << std::strerror(errno);
                freeaddrinfo(servinfo);
                goto close_sock;
            }
            freeaddrinfo(servinfo);

            flag = 1;
            if (::ioctl(m_socket, FIONBIO, (char*)&flag) < 0) {
                SCP_ERR(SCMOD) << "setting socket in blocking mode failed: "
                               << std::strerror(errno);
                return;
            }

            if (::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag))) {
                SCP_WARN(SCMOD) << "setting up TCP_NODELAY option failed: " << std::strerror(errno);
            }
        }

        return;

    close_sock:
        ::close(m_socket);
        m_socket = -1;
    }
};
