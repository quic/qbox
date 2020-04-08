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

#include <cstdio>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <systemc>

#include "backends/net/tap.h"

#if 0
#define MLOG(foo, bar) \
    std::cout << "[" << name() << "] "
#define LOG_F(foo, bar, args...)\
    printf(args)
#else
#include <fstream>
#define MLOG(foo, bar) \
    if (0) std::cout
#define LOG_F(foo, bar, args...)
#endif

using namespace sc_core;

NetworkBackendTap::NetworkBackendTap(sc_core::sc_module_name name, std::string tun)
    : sc_module(name)
{
    m_fd = -1;
    open(tun);

    SC_METHOD(rcv);
    sensitive << m_event;
    dont_initialize();
}

NetworkBackendTap::~NetworkBackendTap()
{
    close();
}

void NetworkBackendTap::open(std::string &tun) {
    struct ifreq ifr;

    if ((m_fd = ::open("/dev/net/tun", O_RDWR)) < 0) {
        MLOG(APP, ERR) << "Failed to open /dev/net/tun: " << strerror(errno) << "\n";
        return;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, tun.c_str(), IFNAMSIZ);
    if(ioctl(m_fd, TUNSETIFF, (void *) &ifr) < 0 ) {
        LOG_F(APP, WRN, "%s: tap device setup failed: %s\n", ifr.ifr_name, strerror(errno));
        ::close(m_fd);
        m_fd = -1;
        return;
    }

    LOG_F(APP, DBG, "TAP opened\n");

    new std::thread(&NetworkBackendTap::rcv_thread, this);
}

void NetworkBackendTap::close() {
    if (m_fd < 0) {
        return;
    }
    ::close(m_fd);
    m_fd = -1;
}

void *NetworkBackendTap::rcv_thread() {
    for (;;) {
        Payload *frame = new Payload(9000);

        int r = ::read(m_fd, frame->data(), (int)frame->capacity());
        if(r > 0) {
            if (r < 60) {
                /*
                 * Pad with zeroes as the minimal payload size is 60 bytes
                 * (60 bytes of data + 4 bytes of crc -> 64bytes)
                 */
                std::memset(frame->data() + r, 0, 60 - r);
                r = 60;
            }
            frame->resize(r);
        }
        else {
            /* error */
            delete frame;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        if (r > 0) {
            MLOG(SIM, TRC) << "frame of size " << r << " EXT -> VP\n";
            m_queue.push(frame);
        }
        if (!m_queue.empty()) {
            m_event.async_notify();
        }
    }
}

void NetworkBackendTap::rcv()
{
    Payload *frame;

    std::lock_guard<std::mutex> lock(m_mutex);

    while (!m_queue.empty()) {
        if (m_can_receive(m_opaque)) {
            frame = m_queue.front();
            m_queue.pop();
            m_receive(m_opaque, *frame);
            delete frame;
        }
        else {
            /* notify myself later, hopefully the queue drains */
            m_event.notify(1, sc_core::SC_MS);
            return;
        }
    }
}

void NetworkBackendTap::send(Payload &frame)
{
    if (m_fd < 0) {
        return;
    }
    MLOG(SIM, TRC) << "frame of size " << frame.size() << " VP -> EXT\n";
    ::write(m_fd, frame.data(), (int)frame.size());
}
