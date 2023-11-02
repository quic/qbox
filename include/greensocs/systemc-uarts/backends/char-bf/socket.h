/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file socket.h
 * @brief socket backend which support biflow socket
 */

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
#include <greensocs/gsutils/ports/biflow-socket.h>

class CharBFBackendSocket : public sc_core::sc_module
{
private:
    SCP_LOGGER();

public:
    gs::biflow_socket<CharBFBackendSocket> socket;

#ifdef WIN32
#pragma message("CharBFBackendSocket not yet implemented for WIN32")
#endif

    std::string type;
    std::string address;
    bool m_server;
    int m_srv_socket;
    bool m_nowait;
    int m_socket = -1;
    uint8_t m_buf[256];

    void start_of_simulation()
    {
        if (type == "tcp") {
            std::string ip = "127.0.0.1";
            std::string port = "4001";

            size_t count = std::count(address.begin(), address.end(), ':');
            if (count == 1) {
                size_t first = address.find_first_of(':');
                ip = address.substr(0, first);
                port = address.substr(first + 1);
            } else {
                SCP_ERR(()) << "malformed address, expecting IP:PORT (e.g 127.0.0.1:4001)";
                return;
            }

            SCP_DEBUG(()) << "IP: " << ip << ", PORT: " << port;

            if (m_server) {
                setup_tcp_server(ip, port);
            } else {
                setup_tcp_client(ip, port);
            }
        } else if (type == "udp") {
            SCP_ERR(()) << "udp sockets are not available in this version";
            return;
        } else if (type == "unix") {
            SCP_ERR(()) << "unix sockets are not available in this version";
            return;
        } else {
            SCP_ERR(()) << "bad value for socket type";
            return;
        }

        new std::thread(&CharBFBackendSocket::rcv_thread, this);
    }
    /**
     * CharBFBackendSocket() - Construct the socket-backend
     * @name: this backend's name
     * @type: socket type, the type of socket coude be tcp , udp or unix
     * @address: socket address
     * @server: type of socket: true if server - false if client
     * @nowait: setting socket in non-blocking mode
     */
    CharBFBackendSocket(sc_core::sc_module_name name, std::string type, std::string address, bool server = true,
                        bool nowait = true)
        : sc_core::sc_module(name)
        , type(type)
        , address(address)
        , m_server(server)
        , m_nowait(nowait)
        , socket("biflow_socket")
    {
        SCP_TRACE(()) << "constructor";
        socket.register_b_transport(this, &CharBFBackendSocket::writefn);
    }

    void end_of_elaboration() { socket.can_receive_any(); }

    void* rcv_thread()
    {
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
                    SCP_ERR(()) << "setting socket in blocking mode failed: " << std::strerror(errno);
                    continue;
                }
#endif

                flag = 1;
                if (::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag))) {
                    SCP_WARN(()) << "setting up TCP_NODELAY option failed: " << std::strerror(errno);
                }

                char str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);
                int cport = ntohs(client_addr.sin_port);

                SCP_DEBUG(()) << "incoming connection from  " << str << ":" << cport;
            } else if (m_nowait && !m_server && m_socket < 0) {
                // TODO
                continue;
            }

            int ret = ::read(m_socket, &m_buf[0], 256);
            if (ret < 0 && errno != EAGAIN && errno != EINTR) {
                SCP_ERR(()) << "read failed: " << std::strerror(errno);
                abort();
            } else if (ret > 0) {
                for (int i = 0; i < ret; i++) {
                    unsigned char c = m_buf[i];
                    socket.enqueue(c);
                }
            }
        }
    }

    void writefn(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        if (!m_nowait && ((!m_server && m_socket < 0) || (m_server && m_srv_socket < 0))) {
            return;
        }

        while (m_nowait && m_socket < 0) {
            SCP_WARN(()) << "Client waiting for server to start";
            sleep(1);
        }

#if 0
        SCP_INFO(()) << "Got " << int(c) << "(" << c << ")";
#endif
        uint8_t* data = txn.get_data_ptr();
        for (int i = 0; i < txn.get_data_length(); i++) {
            if ((::write(m_socket, &data[i], 1)) != 1) {
                SCP_WARN(()) << "Write did not complete: " << strerror(errno);
            }
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

        if (m_nowait) {
#if 0
            flag = 1;
            if(::ioctl(m_srv_socket, FIONBIO, (char *)&flag) < 0) {
                SCP_ERR(()) << "setting socket in non-blocking mode failed: " << std::strerror(errno);
                return;
            }
#endif

            // accept() will be done later in the SC_THREAD
        } else {
            SCP_INFO(()) << "waiting for a connection on " << ip << ":" << port;

            struct sockaddr_in client_addr;

            m_socket = ::accept(m_srv_socket, (struct sockaddr*)&client_addr, &addr_len);
            if (m_socket < 0) {
                ::close(m_srv_socket);
                m_srv_socket = -1;
                SCP_ERR(()) << "accept failed: " << std::strerror(errno);
            }

#if 0
            flag = 1;
            if(::ioctl(m_socket, FIONBIO, (char *)&flag) < 0) {
                SCP_ERR(()) << "setting socket in blocking mode failed: " << std::strerror(errno);
                return;
            }
#endif

            flag = 1;
            if (::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag))) {
                SCP_WARN(()) << "setting up TCP_NODELAY option failed: " << std::strerror(errno);
            }

            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);
            int cport = ntohs(client_addr.sin_port);

            SCP_DEBUG(()) << "incoming connection from  " << str << ":" << cport;
        }
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
            goto close_sock;
        }

        if (m_nowait) {
            flag = 1;
            if (::ioctl(m_socket, FIONBIO, (char*)&flag) < 0) {
                SCP_ERR(()) << "setting socket in non-blocking mode failed: " << std::strerror(errno);
                goto close_sock;
            }

            // connect() will be done later in the SC_THREAD
        } else {
            if (::connect(m_socket, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
                SCP_ERR(()) << "connect failed: " << std::strerror(errno);
                freeaddrinfo(servinfo);
                goto close_sock;
            }
            freeaddrinfo(servinfo);

            flag = 1;
            if (::ioctl(m_socket, FIONBIO, (char*)&flag) < 0) {
                SCP_ERR(()) << "setting socket in blocking mode failed: " << std::strerror(errno);
                return;
            }

            if (::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag))) {
                SCP_WARN(()) << "setting up TCP_NODELAY option failed: " << std::strerror(errno);
            }
        }

        return;

    close_sock:
        ::close(m_socket);
        m_socket = -1;
    }
};
