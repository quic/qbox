/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <virtio_sound_pci.h>

void module_register() { GSC_MODULE_REGISTER_C(virtio_sound_pci, sc_core::sc_object*, sc_core::sc_object*); }
