/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <qemu_hexagon_qtimer.h>

void module_register() { GSC_MODULE_REGISTER_C(qemu_hexagon_qtimer, sc_core::sc_object*); }