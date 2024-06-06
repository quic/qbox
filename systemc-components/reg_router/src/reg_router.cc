/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "reg_router.h"

typedef gs::reg_router<> reg_router;

void module_register() { GSC_MODULE_REGISTER_C(reg_router); }