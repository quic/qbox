/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <vector>

#include <cci_configuration>

#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <virtio/virtio-mmio-gpugl.h>
#include <qemu-instance.h>

#include <module_factory_registery.h>

class virtio_mmio_gpugl : public QemuVirtioMMIOGpuGl
{
public:
    virtio_mmio_gpugl(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : virtio_mmio_gpugl(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    virtio_mmio_gpugl(sc_core::sc_module_name nm, QemuInstance& inst): QemuVirtioMMIOGpuGl(nm, inst) {}
};

extern "C" void module_register();
