/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <riscv32.h>

void module_register() { GSC_MODULE_REGISTER_C(cpu_riscv32, sc_core::sc_object*, uint64_t); }
