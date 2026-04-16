/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <hexagon_tlb.h>

void module_register() { GSC_MODULE_REGISTER_C(hexagon_tlb, sc_core::sc_object*); }
