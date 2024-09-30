/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define SC_ALLOW_DEPRECATED_IEEE_API

#include <module_factory_registery.h>
#include <ports/biflow-socket.h>
namespace gs {
class biflow_router : public sc_core::sc_module
{
public:
    gs::biflow_router_socket<> router;
    biflow_router(sc_core::sc_module_name name): sc_core::sc_module(name), router("socket") {}
};
}; // namespace gs

typedef gs::biflow_router biflow_router;
extern "C" void module_register();
void module_register() { GSC_MODULE_REGISTER_C(biflow_router); }
