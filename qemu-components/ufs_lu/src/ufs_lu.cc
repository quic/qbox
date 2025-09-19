/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ufs_lu.h>

void module_register() { GSC_MODULE_REGISTER_C(ufs_lu, sc_core::sc_object*, sc_core::sc_object*); }