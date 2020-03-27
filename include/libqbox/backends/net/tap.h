/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <queue>
#include <mutex>

#include "../net-backend.h"

#include <libssync/async-event.h>

class NetworkBackendTap : public NetworkBackend, public sc_core::sc_module {
private:
    AsyncEvent m_event;
    std::queue<Payload *> m_queue;
    std::mutex m_mutex;
    int m_fd;

    void open(std::string &tun);
    void *rcv_thread();
    void rcv();
    void close();

public:
    SC_HAS_PROCESS(NetworkBackendTap);
    NetworkBackendTap(sc_core::sc_module_name name, std::string tun);

    virtual ~NetworkBackendTap();

    void send(Payload &frame);
};
