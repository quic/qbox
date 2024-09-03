/*
 * This file is part of libqbox
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
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
#include <virtio/virtio-mmio.h>
#include <qemu-instance.h>

#include <module_factory_registery.h>

class virtio_mmio_sound : public QemuVirtioMMIO
{
    std::string driver;
    std::string audiodev_id;

public:
    virtio_mmio_sound(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : virtio_mmio_sound(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }

    virtio_mmio_sound(sc_core::sc_module_name name, QemuInstance& inst, const std::string& driver = "alsa")
        : QemuVirtioMMIO(name, inst, "virtio-sound-device")
        , driver(driver)
        , audiodev_id(std::string(sc_core::sc_module::name()) + "-id")
    {
        std::stringstream opts;
        opts << driver << ",id=" << audiodev_id;

        m_inst.add_arg("-audiodev");
        m_inst.add_arg(opts.str().c_str());
    }

    void before_end_of_elaboration() override
    {
        QemuVirtioMMIO::before_end_of_elaboration();

        m_dev.set_prop_str("audiodev", audiodev_id.c_str());
    }
};

extern "C" void module_register();
