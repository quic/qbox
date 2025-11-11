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
#include <cstring>
#include <stdio.h>

#include <asio.hpp>
#include <thread>
#include <chrono>
#include <atomic>

#include <scp/report.h>

#include <async_event.h>
#include <uutils.h>
#include <ports/biflow-socket.h>
#include <module_factory_registery.h>

using asio::ip::tcp;
class char_backend_socket : public sc_core::sc_module
{
protected:
    cci::cci_param<std::string> p_address;
    cci::cci_param<bool> p_server;
    cci::cci_param<bool> p_nowait;
    cci::cci_param<bool> p_sigquit;

private:
    asio::io_context m_io_context;
    tcp::acceptor m_acceptor;
    tcp::socket m_asio_socket;
    std::thread m_rcv_thread;

    tcp::endpoint m_local_endpoint;

    static constexpr size_t m_buffer_size = 1024;
    std::array<char, m_buffer_size> m_buffer;

    // Allow only 1 connection at a time
    const int m_tcp_backlog = 1;

    // Flag to signal end of operation to receive thread
    std::atomic<bool> m_stop_rcv_thread = false;

    const std::chrono::milliseconds m_retry_delay = std::chrono::milliseconds(100);

    SCP_LOGGER();

public:
    gs::biflow_socket<char_backend_socket> m_biflow_socket;

    char_backend_socket(sc_core::sc_module_name name)
        : sc_core::sc_module(name)
        , p_address("address", "127.0.0.1:4001", "socket address IP:Port")
        , p_server("server", true, "type of socket: true if server - false if client")
        , p_nowait("nowait", true, "setting socket in non-blocking mode")
        , p_sigquit("sigquit", false, "Interpret 0x1c in the data stream as a sigquit")
        , m_biflow_socket("biflow_socket")
        , m_acceptor(m_io_context)
        , m_asio_socket(m_io_context)
    {
        SCP_TRACE(()) << "char_backend_socket constructor";

        if (!set_endpoint()) {
            SCP_FATAL(()) << "Failed to set local endpoint";
        }

        m_biflow_socket.register_b_transport(this, &char_backend_socket::writefn);
    }

    ~char_backend_socket() { cleanup_receive_thread(); }

    void end_of_simulation() { cleanup_receive_thread(); }

    void cleanup_receive_thread()
    {
        m_stop_rcv_thread = true;
        asio::error_code ignored_error;

        m_asio_socket.close(ignored_error);
        m_acceptor.close(ignored_error);

        if (m_rcv_thread.joinable()) m_rcv_thread.join();
    }

    bool set_endpoint()
    {
        std::string param_ip, param_port;
        asio::ip::address ip;
        unsigned short port;

        size_t count = std::count(p_address.get_value().begin(), p_address.get_value().end(), ':');
        if (count == 1) {
            size_t first = p_address.get_value().find_first_of(':');
            param_ip = p_address.get_value().substr(0, first);
            param_port = p_address.get_value().substr(first + 1);
        } else {
            SCP_ERR(()) << "malformed address, expecting IP:PORT (e.g. 127.0.0.1:4001)";
            return false;
        }

        asio::error_code ec;
        ip = asio::ip::make_address(param_ip, ec);
        if (ec) {
            SCP_ERR(()) << "Invalid IP address: " << param_ip << ", error: " << ec.message();
            return false;
        }

        try {
            port = static_cast<unsigned short>(std::stoi(param_port));
        } catch (const std::invalid_argument& e) {
            SCP_ERR(()) << "Invalid port: " << param_port << " (not a number)";
            return false;
        } catch (const std::out_of_range& e) {
            SCP_ERR(()) << "Port out of range: " << param_port;
            return false;
        }

        m_local_endpoint = tcp::endpoint(ip, port);

        return true;
    }

    void start_of_simulation()
    {
        SCP_DEBUG(()) << "IP: " << m_local_endpoint.address() << ", PORT: " << m_local_endpoint.port();

        if (p_server) {
            setup_tcp_server();
        }

        m_rcv_thread = std::thread(&char_backend_socket::rcv_thread, this);
    }

    void end_of_elaboration() { m_biflow_socket.can_receive_any(); }

    void setup_tcp_server()
    {
        asio::error_code ec;

        m_acceptor.open(m_local_endpoint.protocol(), ec);
        if (ec) {
            SCP_ERR(()) << "Failed to open acceptor: " << ec.message();
            return;
        }

        m_acceptor.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) {
            SCP_ERR(()) << "Failed to set reuse_address option: " << ec.message();
            m_acceptor.close();
            return;
        }

        m_acceptor.bind(m_local_endpoint, ec);
        if (ec) {
            SCP_ERR(()) << "Failed to bind acceptor: " << ec.message();
            m_acceptor.close();
            return;
        }

        m_acceptor.listen(m_tcp_backlog, ec);
        if (ec) {
            SCP_ERR(()) << "Failed to listen on acceptor: " << ec.message();
            m_acceptor.close();
            return;
        }

        m_acceptor.non_blocking(true, ec);
        if (ec) {
            SCP_ERR(()) << "Failed to set acceptor to non-blocking: " << ec.message();
            m_acceptor.close();
            return;
        }
    }

    void writefn(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        while (!m_asio_socket.is_open()) {
            if (p_nowait) {
                return;
            }
            SCP_WARN(()) << "waiting for socket connection on IP address: " << p_address.get_value();
            std::this_thread::sleep_for(m_retry_delay);
        }

        asio::error_code ec;

        uint8_t* data = txn.get_data_ptr();
        size_t bytes_to_send = txn.get_streaming_width();
        size_t bytes_transferred = asio::write(m_asio_socket, asio::buffer(data, bytes_to_send), ec);
        if (ec || (bytes_transferred != bytes_to_send)) {
            if (p_nowait) {
                SCP_WARN(())("(Non blocking) socket closed");
                return;
            } else {
                SCP_WARN(())("(Blocking) socket closed.");
            }
        }
    }

    void rcv_thread()
    {
        try {
            do_receive();
        } catch (const asio::system_error& e) {
            std::cout << "Caught exception: " << e.what() << std::endl;
        }
    }

    void configure_socket()
    {
        asio::error_code ec;

        m_asio_socket.non_blocking(true, ec);
        if (ec) {
            SCP_FATAL(()) << " Failed to set non blocking mode: " << ec.message();
        }

        // Disable Nagle's Algorithm
        m_asio_socket.set_option(asio::ip::tcp::no_delay(true));
        if (ec) {
            SCP_FATAL(()) << " Failed to disable Nagle's Algorithm: " << ec.message();
        }
    }

    void do_receive()
    {
        asio::error_code ec;

        while (!m_stop_rcv_thread) {
            if (p_server) {
                // Server mode
                m_acceptor.accept(m_asio_socket, ec);
                switch (ec.value()) {
                case 0: // Success
                    configure_socket();
                    SCP_DEBUG(()) << "Accepted connection from " << m_asio_socket.remote_endpoint().address() << ":"
                                  << m_asio_socket.remote_endpoint().port();
                    break;
                case asio::error::would_block:
                    SCP_INFO(()) << "Accept failed for non blocking:" << ec.message();
                    std::this_thread::sleep_for(m_retry_delay);
                    break;
                default:
                    SCP_WARN(()) << "Accept connection failed with error:" << ec.message();
                    std::this_thread::sleep_for(m_retry_delay);
                    break;
                }
            } else if (!p_server) {
                // Client mode
                m_asio_socket.connect(m_local_endpoint, ec);
                if (!ec) {
                    configure_socket();
                } else {
                    SCP_INFO(()) << "failed to connect to " << m_local_endpoint.address() << ":"
                                 << m_local_endpoint.port() << " " << ec.message();
                    m_asio_socket.close();
                    std::this_thread::sleep_for(m_retry_delay);
                    continue;
                }
            }

            if (m_asio_socket.is_open()) receive_loop();
        }

        SCP_DEBUG(()) << "Leaving receive thread";
    }

    void receive_loop()
    {
        asio::error_code ec;

        while (m_asio_socket.is_open()) {
            size_t read_count = m_asio_socket.read_some(asio::buffer(m_buffer), ec);
            switch (ec.value()) {
            case 0: // Success
                forward_incoming_data(read_count);
                break;
            case asio::error::would_block:
                // Retry reading after a short delay
                std::this_thread::sleep_for(m_retry_delay);
                break;
            case asio::error::eof:
                SCP_DEBUG(()) << "Remote endpoint disconnected";
                m_asio_socket.close();
                break;
            default:
                SCP_WARN(()) << "Read failed: " << ec.message();
                m_asio_socket.close();
                break;
            }
        }

        SCP_DEBUG(()) << "Leaving receive loop";
    }

    void forward_incoming_data(size_t data_length)
    {
        for (size_t i = 0; i < data_length; i++) {
            unsigned char c = m_buffer[i];
            if (p_sigquit && c == 0x1c) {
                sc_core::sc_stop();
            }
            m_biflow_socket.enqueue(c);
        }
    }
};
extern "C" void module_register();
#endif
