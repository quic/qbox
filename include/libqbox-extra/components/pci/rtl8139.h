/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Greensocs
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LIBQBOX_COMPONENTS_RTL8139_PCI_H
#define _LIBQBOX_COMPONENTS_RTL8139_PCI_H

#include <cci_configuration>

#include <greensocs/libgssync.h>

#include "gpex.h"

class QemuRtl8139Pci : public QemuGPEX::Device
{
    cci::cci_param<std::string> p_mac;
    std::string m_netdev_id;
    cci::cci_param<std::string> p_netdev_str;

public:
    QemuRtl8139Pci(const sc_core::sc_module_name& name, QemuInstance& inst, QemuGPEX* gpex)
        : QemuGPEX::Device(name, inst, "rtl8139")
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
        QemuGPEX::Device::before_end_of_elaboration();
        // if p_mac is empty, a MAC address will be generated for us 
        if(!p_mac.get_value().empty())
            m_dev.set_prop_str("mac", p_mac.get_value().c_str());
        m_dev.set_prop_str("netdev", m_netdev_id.c_str());

        m_dev.set_prop_str("romfile", "");
    }
};
#endif
