/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <opencores_eth.h>

void module_register() { GSC_MODULE_REGISTER_C(opencores_eth, sc_core::sc_object*); }