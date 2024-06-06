/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <riscv-aclint-mtimer.h>

void module_register() { GSC_MODULE_REGISTER_C(riscv_aclint_mtimer, sc_core::sc_object*); }