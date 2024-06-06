/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gs_memory.h"

typedef gs::gs_memory<> gs_memory;

void module_register() { GSC_MODULE_REGISTER_C(gs_memory); }