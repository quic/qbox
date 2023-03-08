/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef GS_BIFLOW_SOCKET_H
#define GS_BIFLOW_SOCKET_H
#include <systemc>
#include <cci_configuration>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

namespace gs {
template <class MODULE, class T = uint8_t>
class biflow_socket : public sc_core::sc_module
{
    SCP_LOGGER();

    uint64_t m_can_send = 0;
    bool infinite = false;
    std::vector<T> m_queue;
    gs::async_event m_send_event;
    std::mutex m_mutex;

    struct ctrl {
        enum { DELTA_CHANGE, ABSOLUTE_VALUE, INFINITE } cmd;
        uint64_t can_send;
    };

    void sendall() {
        std::lock_guard<std::mutex> guard(m_mutex);
        tlm::tlm_generic_payload txn;

        uint64_t sending = (m_can_send <= m_queue.size()) ? m_can_send : m_queue.size();
        if (sending > 0) {
            txn.set_data_length(sending);
            txn.set_data_ptr(&(m_queue[0]));
            txn.set_command(tlm::TLM_IGNORE_COMMAND);
            sc_core::sc_time delay;
            initiator_socket->b_transport(txn, delay);
            m_queue.erase(m_queue.begin(), m_queue.begin() + sending - 1);
            m_can_send -= sending;
        }
    }
    void initiator_ctrl(tlm::tlm_generic_payload& txn, sc_core::sc_time& t) {
        std::lock_guard<std::mutex> guard(m_mutex);
        sc_assert(txn.get_data_length() == sizeof(ctrl));
        ctrl* c = (ctrl*)(txn.get_data_ptr());
        switch (c->cmd) {
        case ctrl::ABSOLUTE_VALUE:
            m_can_send = c->can_send;
            infinite = false;
            break;
        case ctrl::DELTA_CHANGE:
            sc_assert(infinite == false);
            m_can_send += c->can_send;
            infinite = false;
            break;
        case ctrl::INFINITE:
            infinite = true;
        default:
            SCP_FATAL(())("Unkown command");
        }
        m_send_event.notify();
    }
    void send_ctrl(ctrl& c) {
        tlm::tlm_generic_payload txn;
        txn.set_data_length(sizeof(ctrl));
        txn.set_data_ptr(((uint8_t*)&c));
        txn.set_command(tlm::TLM_IGNORE_COMMAND);
        sc_core::sc_time t;
        target_control_socket->b_transport(txn, t);
    }

public:
    tlm_utils::simple_target_socket<MODULE> target_socket;
    tlm_utils::simple_initiator_socket<biflow_socket> target_control_socket;

    tlm_utils::simple_initiator_socket<biflow_socket> initiator_socket;
    tlm_utils::simple_target_socket<biflow_socket> initiator_control_socket;

    SC_HAS_PROCESS(biflow_socket);

    biflow_socket(sc_core::sc_module_name name)
        : sc_core::sc_module(name)
        , target_socket((std::string(name) + "_target_socket").c_str())
        , initiator_socket((std::string(name) + "_initiator_socket").c_str())
        , target_control_socket((std::string(name) + "_target_socket_control").c_str())
        , initiator_control_socket((std::string(name) + "_initiator_socket_control").c_str()) {
        SCP_TRACE(()) << "constructor";

        SC_METHOD(sendall);
        sensitive << m_send_event;
        dont_initialize();
        initiator_control_socket.register_b_transport(this, &biflow_socket::initiator_ctrl);
    }

    template <class M>
    void bind(biflow_socket<M>& other) {
        initiator_socket.bind(other.target_socket);
        target_control_socket.bind(other.initiator_control_socket);
        other.initiator_socket.bind(target_socket);
        other.target_control_socket.bind(initiator_control_socket);
    }

    /* target socket handlng */

    void register_b_transport(MODULE* mod,
                              void (MODULE::*cb)(tlm::tlm_generic_payload&, sc_core::sc_time&)) {
        target_socket.register_b_transport(mod, cb);
    }

    void can_receive_more(int i) {
        ctrl c;
        c.cmd = ctrl::DELTA_CHANGE;
        c.can_send = i;
        send_ctrl(c);
    }
    void can_receive_set(int i) {
        ctrl c;
        c.cmd = ctrl::ABSOLUTE_VALUE;
        c.can_send = i;
        send_ctrl(c);
    }
    void can_receive_any() {
        ctrl c;
        c.cmd = ctrl::INFINITE;
        send_ctrl(c);
    }
    /* Initiator socket handling : THREAD SAFE */
    void enqueue(T data) {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_queue.push_back(data);
        m_send_event.notify();
    }

    void reset() {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_queue.clear();
    }
};
} // namespace gs

#endif