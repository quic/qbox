/*
 * This file is part of libqbox
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "pflash_cfi.h"

void module_register() { GSC_MODULE_REGISTER_C(pflash_cfi, sc_core::sc_object*, uint8_t); }