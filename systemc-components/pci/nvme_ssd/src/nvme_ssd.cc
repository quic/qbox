/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "nvme_ssd.h"

typedef gs::nvme_ssd<> nvme_ssd;

void module_register() { GSC_MODULE_REGISTER_C(nvme_ssd, sc_core::sc_object*); }