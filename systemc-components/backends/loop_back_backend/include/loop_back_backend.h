/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file loop_back_backend.h
 * @brief loop_back_backend for biflow socket
 */

#ifndef _GS_LOOP_BACK_BACKEND_H_
#define _GS_LOOP_BACK_BACKEND_H_

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <systemc>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
#include <uutils.h>
#include <ports/biflow-socket.h>
#include <module_factory_registery.h>

#include <scp/report.h>
class loop_back_backend : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    gs::biflow_socket<loop_back_backend> socket;

    /**
     * loop_back_backend() - Construct the loop_back_backend
     * @name: this backend's name
     */
    loop_back_backend(sc_core::sc_module_name name): sc_core::sc_module(name), socket("biflow_socket")
    {
        SCP_TRACE(()) << "constructor";
        socket.register_b_transport(this, &loop_back_backend::b_transport);
    }

    void end_of_elaboration() { socket.can_receive_any(); }

    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        uint8_t* data = txn.get_data_ptr();
        sc_assert(data != NULL);
        for (unsigned int i = 0; i < txn.get_streaming_width(); i++) {
            socket.enqueue(static_cast<unsigned char>(data[i]));
            SCP_DEBUG(())("loop_back_backend: sending {}", static_cast<char>(data[i]));
        }
    }

    ~loop_back_backend() {}
};
extern "C" void module_register();
#endif
