/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <16550.h>

void module_register() { GSC_MODULE_REGISTER_C(uart_16550, sc_core::sc_object*); }