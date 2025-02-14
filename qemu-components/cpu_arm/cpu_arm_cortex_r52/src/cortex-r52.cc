/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <cortex-r52.h>

void module_register() { GSC_MODULE_REGISTER_C(cpu_arm_cortexR52, sc_core::sc_object*); }