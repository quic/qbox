/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <arm_gicv2m.h>

void module_register() { GSC_MODULE_REGISTER_C(arm_gicv2m, sc_core::sc_object*, sc_core::sc_object*); }
