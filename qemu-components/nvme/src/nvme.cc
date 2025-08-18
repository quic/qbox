/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "nvme.h"

void module_register() { GSC_MODULE_REGISTER_C(nvme_disk, sc_core::sc_object*, sc_core::sc_object*); }