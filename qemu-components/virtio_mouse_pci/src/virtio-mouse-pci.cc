/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <virtio-mouse-pci.h>

void module_register() { GSC_MODULE_REGISTER_C(virtio_mouse_pci, sc_core::sc_object*, sc_core::sc_object*); }