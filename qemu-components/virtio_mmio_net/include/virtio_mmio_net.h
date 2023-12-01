/*
 *  This file is part of libqbox
 *  Copyright (c) 2022 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <vector>

#include <cci_configuration>

#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <virtio/virtio-mmio.h>
#include <qemu-instance.h>

#include <module_factory_registery.h>

class virtio_mmio_net : public QemuVirtioMMIO
{
private:
    std::string netdev_id;
    cci::cci_param<std::string> netdev_str;

public:
    virtio_mmio_net(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : virtio_mmio_net(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    virtio_mmio_net(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuVirtioMMIO(nm, inst, "virtio-net-device")
        , netdev_id(std::string(name()) + "-id")
        , netdev_str("netdev_str", "user,hostfwd=tcp::2222-:22", "netdev string for QEMU (do not specify ID)")
    {
        std::stringstream opts;
        opts << netdev_str.get_value();
        opts << ",id=" << netdev_id;

        m_inst.add_arg("-netdev");
        m_inst.add_arg(opts.str().c_str());
    }

    void before_end_of_elaboration() override
    {
        QemuVirtioMMIO::before_end_of_elaboration();

        m_dev.set_prop_str("netdev", netdev_id.c_str());
    }
};

extern "C" void module_register();
