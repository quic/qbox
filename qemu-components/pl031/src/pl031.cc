/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pl031.h"

void module_register() { GSC_MODULE_REGISTER_C(pl031, sc_core::sc_object*); }