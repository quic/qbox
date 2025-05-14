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
    cci::cci_param<uint8_t> p_outputs;

    void before_end_of_elaboration() override
    {
        qemu_gpex::Device::before_end_of_elaboration();
        get_qemu_dev().set_prop_uint("max_outputs", p_outputs);
    }

    void gpex_realize(qemu::Bus& bus) override { qemu_gpex::Device::gpex_realize(bus); }

    uint8_t outputs() { return p_outputs.get_value(); }

protected:
    QemuVirtioGpu(const sc_core::sc_module_name& name, QemuInstance& inst, const char* gpu_type, qemu_gpex* gpex)
        : QemuVirtioGpu(name, inst, gpu_type, gpex, (const char*)NULL)
    {
    }

    QemuVirtioGpu(const sc_core::sc_module_name& name, QemuInstance& inst, const char* gpu_type, qemu_gpex* gpex,
                  const char* device_id)
        : qemu_gpex::Device(name, inst, gpu_type, device_id), p_outputs("outputs", 1, "Number of outputs for this gpu")
    {
        gpex->add_device(*this);
    }
};
#endif
