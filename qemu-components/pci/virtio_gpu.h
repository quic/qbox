/*
 * This file is part of libqbox
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_GPU_H
#define _LIBQBOX_COMPONENTS_VIRTIO_GPU_H

#include <cci_configuration>

#include <libgssync.h>

#include <qemu_gpex/include/qemu_gpex.h>

class QemuVirtioGpu : public qemu_gpex::Device
{
public:
    void before_end_of_elaboration() override { qemu_gpex::Device::before_end_of_elaboration(); }

    void gpex_realize(qemu::Bus& bus) override { qemu_gpex::Device::gpex_realize(bus); }

protected:
    QemuVirtioGpu(const sc_core::sc_module_name& name, QemuInstance& inst, const char* gpu_type, qemu_gpex* gpex)
        : qemu_gpex::Device(name, inst, (std::string("virtio-gpu-") + gpu_type).c_str())
    {
        gpex->add_device(*this);
    }
};
#endif
