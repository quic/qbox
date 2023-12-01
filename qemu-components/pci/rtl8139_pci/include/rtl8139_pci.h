/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_RTL8139_PCI_H
#define _LIBQBOX_COMPONENTS_RTL8139_PCI_H

#include <cci_configuration>

#include <libgssync.h>
#include <qemu-instance.h>

#include <module_factory_registery.h>

#include <qemu_gpex.h>

class rtl8139_pci : public qemu_gpex::Device
{
    cci::cci_param<std::string> p_mac;
    std::string m_netdev_id;
    cci::cci_param<std::string> p_netdev_str;

public:
    rtl8139_pci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : rtl8139_pci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
    {
    }
    rtl8139_pci(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex)
        : qemu_gpex::Device(name, inst, "rtl8139")
        , p_mac("mac", "", "MAC address of NIC")
        , m_netdev_id(std::string(sc_core::sc_module::name()) + "-id")
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
        // if p_mac is empty, a MAC address will be generated for us
        if (!p_mac.get_value().empty()) m_dev.set_prop_str("mac", p_mac.get_value().c_str());
        m_dev.set_prop_str("netdev", m_netdev_id.c_str());

        m_dev.set_prop_str("romfile", "");
    }
};

extern "C" void module_register();

#endif
