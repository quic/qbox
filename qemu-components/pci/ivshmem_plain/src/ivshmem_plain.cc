/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <ivshmem_plain.h>

void module_register() { GSC_MODULE_REGISTER_C(ivshmem_plain, sc_core::sc_object*, sc_core::sc_object*); }