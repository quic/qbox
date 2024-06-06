/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cortex-a53.h>

void module_register() { GSC_MODULE_REGISTER_C(cpu_arm_cortexA53, sc_core::sc_object*); }