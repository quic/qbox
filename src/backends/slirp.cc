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

#include "backends/net/slirp.h"

#include <systemc>
#include <cstring>

#ifndef _WIN32
#include <sys/time.h>
#endif

static ssize_t net_slirp_send_packet(const void *pkt, size_t pkt_len,
        void *opaque)
{
    NetworkBackendSlirp *nbs = (NetworkBackendSlirp *)opaque;

    nbs->received((const uint8_t *)pkt, pkt_len);

    return pkt_len;
}

static void net_slirp_guest_error(const char *msg, void *opaque)
{
    printf("[NetworkBackendSlirp] guest_error: %s\n", msg);
}

static int64_t net_slirp_clock_get_ns(void *opaque)
{
    /* TODO: add an option to use SystemC time */

#ifndef _WIN32
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000000000LL + (tv.tv_usec * 1000);
#else
    FILETIME ft_now;

    ft_now = GetSystemTimeAsFileTime();

    return (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
#endif
}

static void * net_slirp_timer_new(SlirpTimerCb cb, void *cb_opaque, void *opaque)
{
    printf("[NetworkBackendSlirp] callback not yet implemented: register_timer_new\n");
    exit(1);
    return NULL;
}

static void net_slirp_timer_free(void *timer, void *opaque)
{
    printf("[NetworkBackendSlirp] callback not yet implemented: register_timer_free\n");
    exit(1);
}

static void net_slirp_timer_mod(void *timer, int64_t expire_time, void *opaque)
{
    printf("[NetworkBackendSlirp] callback not yet implemented: register_timer_mod\n");
    exit(1);
}

static void net_slirp_register_poll_fd(int fd, void *opaque)
{
    printf("[NetworkBackendSlirp] callback not yet implemented: register_poll_fd\n");
    exit(1);
}

static void net_slirp_unregister_poll_fd(int fd, void *opaque)
{
    printf("[NetworkBackendSlirp] callback not yet implemented: unregister_poll_fd\n");
    exit(1);
}

static void net_slirp_notify(void *opaque)
{
    printf("[NetworkBackendSlirp] callback not yet implemented: notify\n");
    exit(1);
}

#if 0 /* WIP */
static int slirp_poll_to_epoll(int events)
{
    int ret = 0;

    if (events & SLIRP_POLL_IN) {
        ret |= EPOLLIN;
    }
    if (events & SLIRP_POLL_OUT) {
        ret |= EPOLLOUT;
    }
    if (events & SLIRP_POLL_PRI) {
        ret |= EPOLLPRI;
    }
    if (events & SLIRP_POLL_ERR) {
        ret |= EPOLLERR;
    }
    if (events & SLIRP_POLL_HUP) {
        ret |= EPOLLHUP;
    }

    return ret;
}
#endif

static int net_slirp_add_poll(int fd, int events, void *opaque)
{
#if 1
    printf("net_slirp_add_poll\n");
    exit(1);
    return -1;
#else /* WIP */
    int epoll_fd = *(int *)opaque;
    struct epoll_event *event = new struct epoll_event;

    event->events = slirp_poll_to_epoll(events);
    event->data.fd = fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, event);

    int idx = epoll_events.size();
    epoll_events.push_back(event);
    return idx;
#endif
}

NetworkBackendSlirp::NetworkBackendSlirp(sc_core::sc_module_name name)
{
    memset(&m_cfg, 0, sizeof(m_cfg));

    m_cfg.version = 1;

    m_cfg.restricted = 0;

    m_cfg.in_enabled = true;
    m_cfg.vnetwork = { .s_addr = htonl(0x0a000200) }; /* 10.0.2.0 */
    m_cfg.vnetmask = { .s_addr = htonl(0xffffff00) }; /* 255.255.255.0 */
    m_cfg.vhost = { .s_addr = htonl(0x0a000202) }; /* 10.0.2.2 */
    m_cfg.vdhcp_start = { .s_addr = htonl(0x0a00020f) }; /* 10.0.2.15 */
    m_cfg.vnameserver = { .s_addr = htonl(0x0a000203) }; /* 10.0.2.3 */

    m_cfg.in6_enabled = false;

    memset(&m_cb, 0, sizeof(m_cb));

    m_cb.send_packet = net_slirp_send_packet;
    m_cb.guest_error = net_slirp_guest_error;
    m_cb.clock_get_ns = net_slirp_clock_get_ns;
    m_cb.timer_new = net_slirp_timer_new;
    m_cb.timer_free = net_slirp_timer_free;
    m_cb.timer_mod = net_slirp_timer_mod;
    m_cb.register_poll_fd = net_slirp_register_poll_fd;
    m_cb.unregister_poll_fd = net_slirp_unregister_poll_fd;
    m_cb.notify = net_slirp_notify;

    m_slirp = slirp_new(&m_cfg, &m_cb, this);

    m_timeout = 0xffffffff;
#if 0 /* WIP */
    int epoll_fd = epoll_create1(0);
    slirp_pollfds_fill(m_slirp, &m_timeout, net_slirp_add_poll, &epoll_fd);
#else
    slirp_pollfds_fill(m_slirp, &m_timeout, net_slirp_add_poll, NULL);
#endif

    SC_METHOD(rcv);
    sensitive << m_event;
    dont_initialize();
}

NetworkBackendSlirp::~NetworkBackendSlirp()
{
}

void NetworkBackendSlirp::rcv()
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

void NetworkBackendSlirp::send(Payload &frame)
{
    slirp_input(m_slirp, frame.data(), frame.size());
}

void NetworkBackendSlirp::received(const uint8_t *data, int size)
{
    Payload *frame = new Payload(9000);

    memcpy(frame->data(), data, size);
    frame->resize(size);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_queue.push(frame);
    m_event.async_notify();
}
