/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "char_backend_stdio.h"
struct termios char_backend_stdio::oldtty;
bool char_backend_stdio::oldtty_valid = false;

void module_register() { GSC_MODULE_REGISTER_C(char_backend_stdio); }