/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <systemc>

#include <sifive-x280-rtl.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(cpu_sifive_X280, sc_core::sc_object*, /*char* */ std::string, uint64_t);
}