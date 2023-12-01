/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @class KeepAlive
 * @brief this calss is used to keep the simulation alive by using an async event
 */
#ifndef _GS_KEEP_ALIVE_H_
#define _GS_KEEP_ALIVE_H_

#include <systemc>
#include <async_event.h>
#include <scp/report.h>
#include <module_factory_registery.h>

class keep_alive : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    gs::async_event keep_alive_event;

    keep_alive(sc_core::sc_module_name name): sc_core::sc_module(name)
    {
        SCP_DEBUG(()) << "keep_alive: Constructor";
        keep_alive_event.async_attach_suspending();
    }
    ~keep_alive() {}
};

extern "C" void module_register();

#endif
