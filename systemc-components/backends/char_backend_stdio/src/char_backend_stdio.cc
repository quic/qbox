/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "char_backend_stdio.h"

descriptor_t char_backend_stdio::stdin_fd;
console_mode_t char_backend_stdio::old_console_mode;
bool char_backend_stdio::old_console_mode_valid = false;

void module_register() { GSC_MODULE_REGISTER_C(char_backend_stdio); }