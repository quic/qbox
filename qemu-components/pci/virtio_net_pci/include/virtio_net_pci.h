/*
 * This file is part of libqbox
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_NET_PCI_H
#define _LIBQBOX_COMPONENTS_VIRTIO_NET_PCI_H

#include <cci_configuration>

#include <libgssync.h>
#include <qemu-instance.h>

#include <module_factory_registery.h>

#include <qemu_gpex.h>

class virtio_net_pci : public qemu_gpex::Device
{
    std::string m_netdev_id;
    cci::cci_param<std::string> p_mac;
    cci::cci_param<std::string> p_netdev_str;

public:
    virtio_net_pci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : virtio_net_pci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
    {
    }
    virtio_net_pci(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex)
        : qemu_gpex::Device(name, inst, "virtio-net-pci")
        , m_netdev_id(std::string(sc_core::sc_module::name()) + "-id")
        , p_mac("mac", "", "MAC address of NIC")
        , p_netdev_str("netdev_str", "type=user", "netdev string for QEMU (do not specify ID)")
    {
        std::stringstream opts;
        opts << p_netdev_str.get_value();
        opts << ",id=" << m_netdev_id;

        m_inst.add_arg("-netdev");
        m_inst.add_arg(opts.str().c_str());

        gpex->add_device(*this);
    }

    void before_end_of_elaboration() override
    {
        qemu_gpex::Device::before_end_of_elaboration();
        if (!p_mac.get_value().empty()) m_dev.set_prop_str("mac", p_mac.get_value().c_str());
        m_dev.set_prop_str("netdev", m_netdev_id.c_str());
        m_dev.set_prop_str("romfile", "");
    }
};

extern "C" void module_register();

#endif
