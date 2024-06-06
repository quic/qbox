/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arm-smmu.h>

void module_register() { GSC_MODULE_REGISTER_C(arm_smmu, sc_core::sc_object*); }