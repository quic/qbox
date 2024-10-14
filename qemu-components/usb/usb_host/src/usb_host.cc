/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <usb_host.h>

void module_register() { GSC_MODULE_REGISTER_C(usb_host, sc_core::sc_object*, sc_core::sc_object*); }