/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hexagon-l2vic.h>

void module_register() { GSC_MODULE_REGISTER_C(hexagon_l2vic, sc_core::sc_object*); }