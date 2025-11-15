/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "gs_gpex.h"

typedef gs::gs_gpex<> gs_gpex;

void module_register() { GSC_MODULE_REGISTER_C(gs_gpex); }