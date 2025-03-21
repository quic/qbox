/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <cortex-a710.h>

void module_register() { GSC_MODULE_REGISTER_C(cpu_arm_cortexA710, sc_core::sc_object*); }