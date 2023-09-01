/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_GPU_H
#define _LIBQBOX_COMPONENTS_VIRTIO_GPU_H

#include <cci_configuration>

#include <greensocs/libgssync.h>

#include "gpex.h"

class QemuVirtioGpu : public QemuGPEX::Device
{
public:
    void before_end_of_elaboration() override { QemuGPEX::Device::before_end_of_elaboration(); }

    void gpex_realize(qemu::Bus& bus) override { QemuGPEX::Device::gpex_realize(bus); }

protected:
    QemuVirtioGpu(const sc_core::sc_module_name& name, QemuInstance& inst, const char* gpu_type)
        : QemuGPEX::Device(name, inst, (std::string("virtio-gpu-") + gpu_type).c_str())
    {
    }
};
#endif
