/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <hexagon_globalreg.h>

void module_register() { GSC_MODULE_REGISTER_C(hexagon_globalreg, sc_core::sc_object*); }
