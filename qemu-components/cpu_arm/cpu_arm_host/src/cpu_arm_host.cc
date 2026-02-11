/*
 * This file is part of libqbox
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <cpu_arm_host.h>

void module_register() { GSC_MODULE_REGISTER_C(cpu_arm_host, sc_core::sc_object*); }