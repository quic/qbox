/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include "reg_router.h"

typedef gs::reg_router<> reg_router;

void module_register() { GSC_MODULE_REGISTER_C(reg_router); }