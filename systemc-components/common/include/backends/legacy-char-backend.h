/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <systemc>
#include <cci_configuration>
#include <scp/report.h>
#include <ports/biflow-socket.h>

#pragma once

class LegacyCharBackend : public sc_core::sc_module
{
protected:
    void* m_opaque;
    void (*m_receive)(void* opaque, const uint8_t* buf, int size);
    int (*m_can_receive)(void* opaque);

    gs::biflow_socket<LegacyCharBackend> socket;

public:
    SCP_LOGGER();
    static void recieve(void* opaque, const uint8_t* buf, int size)
    {
        LegacyCharBackend* t = (LegacyCharBackend*)opaque;
        for (int i = 0; i < size; i++) {
            t->socket.enqueue(buf[i]);
        }
    }

    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        uint8_t* data = txn.get_data_ptr();
        for (int i = 0; i < txn.get_streaming_width(); i++) {
            write(data[i]);
        }
    }
    LegacyCharBackend(): socket("biflow_socket")
    {
        SCP_DEBUG("LegacyCharBackend") << "LegacyCharBackend constructor";
        m_opaque = this;
        m_receive = recieve;
        m_can_receive = can_receive;
        socket.register_b_transport(this, &LegacyCharBackend::b_transport);
    }

    void start_of_simulation() { socket.can_receive_any(); }

    virtual void write(unsigned char c) = 0;

    static int can_receive(void* opaque) { return 1; }

    void register_receive(void* opaque, void (*receive)(void* opaque, const uint8_t* buf, int size),
                          int (*can_receive)(void* opaque))
    {
        SCP_FATAL(())("Deprecated!!!!");
        m_opaque = opaque;
        m_receive = receive;
        m_can_receive = can_receive;
    }
};
