/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <mutex>
#include <queue>

#include "../net-backend.h"

#include <greensocs/libgssync/async_event.h>

class NetworkBackendTap : public NetworkBackend, public sc_core::sc_module
{
private:
    gs::async_event m_event;
    std::queue<Payload*> m_queue;
    std::mutex m_mutex;
    int m_fd;

    void open(std::string& tun);
    void* rcv_thread();
    void rcv();
    void close();

public:
    SC_HAS_PROCESS(NetworkBackendTap);
    NetworkBackendTap(sc_core::sc_module_name name, std::string tun);

    virtual ~NetworkBackendTap();

    void send(Payload& frame);
};
