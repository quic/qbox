/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <arm_gicv3_its.h>

void module_register() { GSC_MODULE_REGISTER_C(arm_gicv3_its, sc_core::sc_object*, sc_core::sc_object*); }