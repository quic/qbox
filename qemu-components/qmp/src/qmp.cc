/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <qmp.h>

void module_register() { GSC_MODULE_REGISTER_C(qmp, sc_core::sc_object*); }