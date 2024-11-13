/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EXITER_H
#define EXITER_H

#include <systemc>
#include <scp/report.h>
#include <cci_configuration>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include <tlm_sockets_buswidth.h>
#include <module_factory_registery.h>

/**
 * @brief exiter: sc_module that when writen to, exits !
 */
SC_MODULE (exiter) {
    SCP_LOGGER();

protected:
    cci::cci_param<uint32_t> exit_code;
    tlm_utils::simple_target_socket<exiter, DEFAULT_TLM_BUSWIDTH> socket;

public:
    void b_transport(tlm::tlm_generic_payload & trans, sc_core::sc_time & delay)
    {
        switch (trans.get_command()) {
        case tlm::TLM_READ_COMMAND: {
            uint32_t data = 0;
            *reinterpret_cast<uint32_t*>(trans.get_data_ptr()) = 0;
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
            break;
        }
        case tlm::TLM_WRITE_COMMAND: {
            exit_code = *reinterpret_cast<uint32_t*>(trans.get_data_ptr());
            sc_core::sc_stop();
            break;
        }
        default:
            SCP_ERR(()) << "TLM command not supported";
            break;
        }
    }

    SC_CTOR (exiter)
        : exit_code("exit_code", 0, "Exit code returned by writing to the exiter memory location")
        , socket("target_socket")
        {
            socket.register_b_transport(this, &exiter::b_transport);
        }
};

extern "C" void module_register();

#endif //  EXITER_H
