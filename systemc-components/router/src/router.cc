/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "router.h"

typedef gs::router<> router;

void module_register() { GSC_MODULE_REGISTER_C(router); }