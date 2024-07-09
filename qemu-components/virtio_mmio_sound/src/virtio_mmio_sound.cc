/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <virtio_mmio_sound.h>

void module_register() { GSC_MODULE_REGISTER_C(virtio_mmio_sound, sc_core::sc_object*); }
