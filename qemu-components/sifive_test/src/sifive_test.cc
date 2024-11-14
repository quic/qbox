/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sifive_test.h>

void module_register() { GSC_MODULE_REGISTER_C(sifive_test, sc_core::sc_object*); }