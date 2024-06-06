/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <neoverse-n2.h>

void module_register() { GSC_MODULE_REGISTER_C(cpu_arm_neoverseN2, sc_core::sc_object*); }
