/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <uart-ibex.h>

void module_register() { GSC_MODULE_REGISTER_C(ibex_uart); }