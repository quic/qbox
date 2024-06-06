/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <qemu_tablet.h>

void module_register() { GSC_MODULE_REGISTER_C(qemu_tablet, sc_core::sc_object*, sc_core::sc_object*); }