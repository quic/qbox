/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Bi-directional Flow controlled socket.
 *
 * The bi-directional flow controlled socket is intended to be used between devices and serial-like
 * back-ends.
 *
 * The socket transports 'standard' Generic Protocol TLM packets, but _ONLY_ makes use of the data
 * component. The socket is templated on a type (T) which is the unit of data that will be
 * transmitted. (Default is uint8)
 *
 * The interface allows the socket owner to allow a specific, or unlimited amount of data to arrive.
 * Hence the owner may specify that only one item can arrive (hence use a send/acknowledge
 * protocol), or it may use a limited buffer protocol (typical of a uart for instance) or, for
 * instance, for an output to a host, which itself is normally buffered, the socket may be set to
 * receive unlimited data.
 *
 * # **** NOTICE : async_detach_suspending / async_attach_suspending ****
 * The bidirectional socket will own an async_event, 'waiting' for the other side of the
 * communication.
 * - In the case of ZERO, _or_ UNLIMITED data, the expectation is that the async_event should _NOT_
 * cause systemC to wait for external events
 * - In the case of an absolute value above 0, the expectation is that SystemC _SHOULD_ wait for
 * external events.
 *
 * NOTE: Hence, all sockets start DETACHED, and will only be attached if/when a non-zero absolute
 * value is received from the other side.
 */

#ifndef GS_BIFLOW_SOCKET_H
#define GS_BIFLOW_SOCKET_H
#include <systemc>
#include <cci_configuration>
#include <scp/report.h>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_sockets_buswidth.h>
#include <async_event.h>

namespace gs {
struct biflow_bindable {
    virtual void bind(biflow_bindable& other) = 0;

    virtual tlm::tlm_target_socket<DEFAULT_TLM_BUSWIDTH>& get_target_socket() = 0;
    virtual tlm::tlm_initiator_socket<DEFAULT_TLM_BUSWIDTH>& get_initiator_socket() = 0;
    virtual tlm::tlm_initiator_socket<DEFAULT_TLM_BUSWIDTH>& get_target_control_socket() = 0;
    virtual tlm::tlm_target_socket<DEFAULT_TLM_BUSWIDTH>& get_initiator_control_socket() = 0;
};

template <class MODULE, class T = uint8_t>
class biflow_socket : public sc_core::sc_module, public biflow_bindable
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

    void sendall()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        tlm::tlm_generic_payload txn;

        uint64_t sending = (infinite || (m_can_send > m_queue.size())) ? m_queue.size() : m_can_send;
        if (sending > 0) {
            txn.set_data_length(sending);
            txn.set_data_ptr(&(m_queue[0]));
            txn.set_command(tlm::TLM_IGNORE_COMMAND);
            sc_core::sc_time delay;
            initiator_socket->b_transport(txn, delay);
            m_queue.erase(m_queue.begin(), m_queue.begin() + sending);
            m_can_send -= sending;
        }
    }
    void initiator_ctrl(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        sc_assert(txn.get_data_length() == sizeof(ctrl));
        ctrl* c = (ctrl*)(txn.get_data_ptr());
        switch (c->cmd) {
        case ctrl::ABSOLUTE_VALUE:
            m_can_send = c->can_send;
            infinite = false;

            // If the other side signals a specific number, then it must be waiting on interrupts
            // etc to drive this, so lets interpret that as a 'signal' that we should keep the
            // suspending flag. If this is set to 0, then the other side stopped listening
            if (m_can_send)
                m_send_event.async_attach_suspending();
            else
                m_send_event.async_detach_suspending();
            break;
        case ctrl::DELTA_CHANGE:
            sc_assert(infinite == false);
            m_can_send += c->can_send;
            infinite = false;
            break;
        case ctrl::INFINITE:
            infinite = true;
            break;
        default:
            SCP_FATAL(())("Unkown command");
        }
        m_send_event.notify();
    }
    void send_ctrl(ctrl& c)
    {
        tlm::tlm_generic_payload txn;
        txn.set_data_length(sizeof(ctrl));
        txn.set_data_ptr(((uint8_t*)&c));
        txn.set_command(tlm::TLM_IGNORE_COMMAND);
        sc_core::sc_time t;
        target_control_socket->b_transport(txn, t);
    }

public:
    tlm_utils::simple_target_socket<MODULE, DEFAULT_TLM_BUSWIDTH> target_socket;
    tlm_utils::simple_initiator_socket<biflow_socket, DEFAULT_TLM_BUSWIDTH> target_control_socket;

    tlm_utils::simple_initiator_socket<biflow_socket, DEFAULT_TLM_BUSWIDTH> initiator_socket;
    tlm_utils::simple_target_socket<biflow_socket, DEFAULT_TLM_BUSWIDTH> initiator_control_socket;

    SC_HAS_PROCESS(biflow_socket);

    /**
     * @brief Construct a new biflow socket object
     *
     * @param name
     */
    biflow_socket(sc_core::sc_module_name name)
        : sc_core::sc_module(name)
        , target_socket((std::string(name) + "_target_socket").c_str())
        , initiator_socket((std::string(name) + "_initiator_socket").c_str())
        , target_control_socket((std::string(name) + "_target_socket_control").c_str())
        , initiator_control_socket((std::string(name) + "_initiator_socket_control").c_str())
    {
        SCP_TRACE(()) << "constructor";

        SC_METHOD(sendall);
        sensitive << m_send_event;
        dont_initialize();
        initiator_control_socket.register_b_transport(this, &biflow_socket::initiator_ctrl);
        m_send_event.async_detach_suspending();
    }

    tlm::tlm_target_socket<DEFAULT_TLM_BUSWIDTH>& get_target_socket() { return target_socket; }
    tlm::tlm_initiator_socket<DEFAULT_TLM_BUSWIDTH>& get_initiator_socket() { return initiator_socket; };
    tlm::tlm_initiator_socket<DEFAULT_TLM_BUSWIDTH>& get_target_control_socket() { return target_control_socket; };
    tlm::tlm_target_socket<DEFAULT_TLM_BUSWIDTH>& get_initiator_control_socket() { return initiator_control_socket; };
    /**
     * @brief Bind method to connect two biflow sockets
     *
     * @tparam M
     * @param other : other socket
     */
    void bind(biflow_bindable& other)
    {
        initiator_socket.bind(other.get_target_socket());
        target_control_socket.bind(other.get_initiator_control_socket());
        other.get_initiator_socket().bind(target_socket);
        other.get_target_control_socket().bind(initiator_control_socket);
    }

    /* target socket handlng */

    /**
     * @brief Register b_transport to be called whenever data is received from the socket.
     *
     * NOTE: this strips the 'quantum time' from the normal b_transport as it makes no sense in this
     * case.
     *
     * @param mod
     * @param cb
     */
    void register_b_transport(MODULE* mod, void (MODULE::*cb)(tlm::tlm_generic_payload&, sc_core::sc_time&))
    {
        target_socket.register_b_transport(mod, cb);
    }

    /**
     * @brief can_receive_more
     *
     * @param i number of _additional_ items that can now be received.
     */
    void can_receive_more(int i)
    {
        ctrl c;
        c.cmd = ctrl::DELTA_CHANGE;
        c.can_send = i;
        send_ctrl(c);
    }
    /**
     * @brief can_receive_set
     *
     * @param i number of items that can now be received.
     *
     * Setting this to zero will have the side effect of async_detach_suspending the other ends
     * async_event. Setting this to NON-zero will have the side effect of async_attach_suspending
     * the other ends async_event.
     */
    void can_receive_set(int i)
    {
        ctrl c;
        c.cmd = ctrl::ABSOLUTE_VALUE;
        c.can_send = i;
        send_ctrl(c);
    }
    /**
     * @brief can_receive_any
     * Allow unlimited items to arrive.
     */
    void can_receive_any()
    {
        ctrl c;
        c.cmd = ctrl::INFINITE;
        send_ctrl(c);
    }

    /**
     * @brief enqueue
     * Enqueue data to be sent (unlimited queue size)
     * NOTE: Thread safe.
     * @param data
     */
    void enqueue(T data)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_queue.push_back(data);
        m_send_event.notify();
    }

    /**
     * @brief reset
     * Clear the current send queue.
     * NOTE: Thread safe.
     */
    void reset()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_queue.clear();
    }
};
} // namespace gs

#endif
