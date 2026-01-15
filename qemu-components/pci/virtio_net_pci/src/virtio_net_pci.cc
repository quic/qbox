/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <virtio_net_pci.h>

void module_register() { GSC_MODULE_REGISTER_C(virtio_net_pci, sc_core::sc_object*, sc_core::sc_object*); }