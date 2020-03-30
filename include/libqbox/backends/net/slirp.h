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

#include <slirp/libslirp.h>

class NetworkBackendSlirp : public NetworkBackend, public sc_core::sc_module {
private:
    AsyncEvent m_event;
    std::queue<Payload *> m_queue;
    std::mutex m_mutex;

    SlirpConfig m_cfg;
    SlirpCb m_cb;
    Slirp *m_slirp;
    uint32_t m_timeout;

    void rcv();

public:
    SC_HAS_PROCESS(NetworkBackendSlirp);
    NetworkBackendSlirp(sc_core::sc_module_name name);

    virtual ~NetworkBackendSlirp();

    void send(Payload &frame);
    void received(const uint8_t *data, int size);
};
