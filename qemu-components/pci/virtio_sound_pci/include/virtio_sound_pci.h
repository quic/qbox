/*
 * This file is part of libqbox
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <cci_configuration>

#include <libgssync.h>
#include <qemu-instance.h>

#include <module_factory_registery.h>

#include <qemu_gpex.h>

class virtio_sound_pci : public qemu_gpex::Device
{
    std::string driver;
    std::string audiodev_id;

public:
    virtio_sound_pci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : virtio_sound_pci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
    {
    }

    virtio_sound_pci(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex,
                     const std::string& driver = "alsa")
        : qemu_gpex::Device(name, inst, "virtio-sound-pci")
        , driver(driver)
        , audiodev_id(std::string(sc_core::sc_module::name()) + "-id")
    {
        std::stringstream opts;
        opts << driver << ",id=" << audiodev_id;

        m_inst.add_arg("-audiodev");
        m_inst.add_arg(opts.str().c_str());

        gpex->add_device(*this);
    }

    void before_end_of_elaboration() override
    {
        qemu_gpex::Device::before_end_of_elaboration();

        m_dev.set_prop_str("audiodev", audiodev_id.c_str());
    }
};

extern "C" void module_register();
