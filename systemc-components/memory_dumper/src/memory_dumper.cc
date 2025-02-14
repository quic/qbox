/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <memory_dumper.h>

typedef gs::memory_dumper<> memory_dumper;

void module_register() { GSC_MODULE_REGISTER_C(memory_dumper); }