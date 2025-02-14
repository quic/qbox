/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "qemu_pl011.h"

void module_register() { GSC_MODULE_REGISTER_C(qemu_pl011, sc_core::sc_object*); }