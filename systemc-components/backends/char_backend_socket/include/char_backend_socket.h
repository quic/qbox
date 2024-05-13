/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file socket.h
 * @brief socket backend which support biflow socket
 */

#ifndef _GS_UART_BACKEND_SOCKET_H_
#define _GS_UART_BACKEND_SOCKET_H_

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

#include <async_event.h>
#include <uutils.h>
#include <ports/biflow-socket.h>
#include <module_factory_registery.h>

class char_backend_socket : public sc_core::sc_module
{
protected:
    cci::cci_param<std::string> p_address;
    cci::cci_param<bool> p_server;
    cci::cci_param<bool> p_nowait;
    cci::cci_param<bool> p_sigquit;

private:
    SCP_LOGGER();
    std::string ip;
    std::string port;

public:
    gs::biflow_socket<char_backend_socket> socket;

#ifdef WIN32
#pragma message("char_backend_socket not yet implemented for WIN32")
#endif

    int m_srv_socket;
    int m_socket = -1;
    uint8_t m_buf[256];

    void sock_setup()
    {
        int flags = fcntl(m_socket, F_GETFL, 0);
        flags |= O_NONBLOCK;
        if (::fcntl(m_socket, F_SETFL, flags) != 0) {
            SCP_ERR(()) << "setting socket in non-blocking mode failed: " << std::strerror(errno);
        }

        flags = 1;
        if (::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags))) {
            SCP_WARN(()) << "setting up TCP_NODELAY option failed: " << std::strerror(errno);
        }
    }

    void start_of_simulation()
    {
        size_t count = std::count(p_address.get_value().begin(), p_address.get_value().end(), ':');
        if (count == 1) {
            size_t first = p_address.get_value().find_first_of(':');
            ip = p_address.get_value().substr(0, first);
            port = p_address.get_value().substr(first + 1);
        } else {
            SCP_ERR(()) << "malformed address, expecting IP:PORT (e.g. 127.0.0.1:4001)";
            return;
        }

        SCP_DEBUG(()) << "IP: " << ip << ", PORT: " << port;

        if (p_server) {
            setup_tcp_server(ip, port);
        } else {
            setup_tcp_client(ip, port);
        }

        new std::thread(&char_backend_socket::rcv_thread, this);
    }

    char_backend_socket(sc_core::sc_module_name name)
        : sc_core::sc_module(name)
        , p_address("address", "127.0.0.1:4001", "socket address IP:Port")
        , p_server("server", true, "type of socket: true if server - false if client")
        , p_nowait("nowait", true, "setting socket in non-blocking mode")
        , p_sigquit("sigquit", false, "Interpret 0x1c in the data stream as a sigquit")
        , socket("biflow_socket")
    {
        SCP_TRACE(()) << "char_backend_socket constructor";
        socket.register_b_transport(this, &char_backend_socket::writefn);
    }

    void end_of_elaboration() { socket.can_receive_any(); }

    void* rcv_thread()
    {
        for (;;) {
            if (p_server && m_socket < 0) {
                socklen_t addr_len = sizeof(struct sockaddr_in);
                struct sockaddr_in client_addr;
                int flag;

                m_socket = ::accept(m_srv_socket, (struct sockaddr*)&client_addr, &addr_len);
                if (m_socket < 0) {
                    continue;
                }
                sock_setup();

                char str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);
                int cport = ntohs(client_addr.sin_port);

                SCP_DEBUG(()) << "incoming connection from  " << str << ":" << cport;
            } else if (!p_server && m_socket < 0) {
                SCP_DEBUG(())("Waiting for connection");
                sleep(1);
                setup_tcp_client(ip, port);
                continue;
            }

            struct pollfd fd;
            int ret;
            fd.fd = m_socket;
            fd.events = POLLIN;
            if (poll(&fd, 1, 1000) == 0) {
                continue;
            }
            if (fd.revents > 0x4) {
                close(m_socket);
                m_socket = -1;
                if (!p_nowait) {
                    SCP_FATAL(())("Non waiting Socket closed");
                } else {
                    SCP_WARN(())("Socket closed, will wait for new connection");
                }
                continue;
            }
            ret = ::read(m_socket, m_buf, 1); // We can only guarantee 1 read will work
            if (ret > 0) {
                for (int i = 0; i < ret; i++) {
                    unsigned char c = m_buf[i];
                    if (p_sigquit && c == 0x1c) {
                        sc_core::sc_stop();
                    }
                    socket.enqueue(c);
                }
            }
        }
    }

    void writefn(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        while (m_socket < 0) {
            if (p_nowait) {
                return;
            }
            SCP_WARN(()) << "waiting for socket connection on IP address: " << p_address.get_value();
            sleep(1);
        }

        uint8_t* data = txn.get_data_ptr();
        for (int i = 0; i < txn.get_streaming_width(); i++) {
            do {
                if ((::write(m_socket, &data[i], 1)) != 1) {
                    if (errno == EAGAIN) {
                        SCP_WARN(())("(Blocking) write did not complete (EAGAIN)");
                        sleep(1);
                    } else {
                        if (p_nowait) {
                            SCP_WARN(())("(Non blocking) socket closed");
                            return;
                        } else {
                            SCP_FATAL(())("(Blocking) socket closed.");
                        }
                    }
                } else {
                    break; // Write completed normally
                }
            } while (true);
        }
    }

    void setup_tcp_server(std::string ip, std::string port)
    {
        int ret;
        struct sockaddr_in addr_in;
        int iport;
        int flag;

        SCP_DEBUG(()) << "setting up TCP server on " << ip << ":" << port;

        socklen_t addr_len = sizeof(struct sockaddr_in);

        m_srv_socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_srv_socket < 0) {
            SCP_ERR(()) << "socket failed: " << std::strerror(errno);
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
            SCP_ERR(()) << "bind failed: " << std::strerror(errno);
            return;
        }

        ret = listen(m_srv_socket, 1);
        if (ret < 0) {
            ::close(m_srv_socket);
            m_srv_socket = -1;
            SCP_ERR(()) << "listen failed: " << std::strerror(errno);
            return;
        }
        // the connection will be done in the rcv_thread
    }

    void setup_tcp_client(std::string ip, std::string port)
    {
        int flag = 1;
        int status;
        struct addrinfo hints;
        struct addrinfo* servinfo;

        SCP_INFO(()) << "setting up TCP client connection to " << ip << ":" << port;

        m_socket = ::socket(AF_INET, SOCK_STREAM, 0);

        if (m_socket == -1) {
            SCP_ERR(()) << "socket failed: " << std::strerror(errno);
            return;
        }

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        status = getaddrinfo(ip.c_str(), port.c_str(), &hints, &servinfo);
        if (status == -1) {
            SCP_ERR(()) << "getaddrinfo failed: " << std::strerror(errno);
            close_sock();
        }

        if (::connect(m_socket, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
            SCP_ERR(()) << "connect failed: " << std::strerror(errno);
            freeaddrinfo(servinfo);
            close_sock();
        }
        freeaddrinfo(servinfo);

        sock_setup();

        return;
    }

    void close_sock()
    {
        ::close(m_socket);
        m_socket = -1;
    }
};
extern "C" void module_register();
#endif