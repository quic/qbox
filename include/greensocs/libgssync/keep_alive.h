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
#include <greensocs/libgssync/async_event.h>
#include <scp/report.h>
#include <greensocs/gsutils/module_factory_registery.h>

class KeepAlive : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    gs::async_event keep_alive_event;

    KeepAlive(sc_core::sc_module_name name): sc_core::sc_module(name)
    {
        SCP_DEBUG(()) << "KeepAlive: Constructor";
        keep_alive_event.async_attach_suspending();
    }
    ~KeepAlive() {}
};
GSC_MODULE_REGISTER(KeepAlive);

#endif
