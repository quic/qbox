/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
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

#ifndef _LIBQBOX_COMPONENTS_OPENCORES_ETH_H
#define _LIBQBOX_COMPONENTS_OPENCORES_ETH_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "libqbox/components/device.h"
#include "libqbox/ports/target.h"
#include "libqbox/ports/initiator-signal-socket.h"

class QemuOpencoresEth : public QemuDevice
{
protected:
    cci::cci_param<std::string> p_mac;
    std::string m_netdev_id;
    cci::cci_param<std::string> p_netdev_str;

public:
    QemuTargetSocket<> regs_socket;
    QemuTargetSocket<> desc_socket;
    QemuInitiatorSignalSocket irq_out;

    QemuOpencoresEth(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : QemuOpencoresEth(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    QemuOpencoresEth(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "open_eth")
        , p_mac("mac", "00:11:22:33:44:55", "MAC address of NIC")
        , m_netdev_id(std::string(name()) + "-id")
        , p_netdev_str("netdev_str", "user,hostfwd=tcp::2222-:22", "netdev string for QEMU (do not specify ID)")
        , regs_socket("regs", inst)
        , desc_socket("desc", inst)
        , irq_out("irq_out")
    {
        std::stringstream opts;
        opts << p_netdev_str.get_value();
        opts << ",id=" << m_netdev_id;

        m_inst.add_arg("-netdev");
        m_inst.add_arg(opts.str().c_str());
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_str("mac", p_mac.get_value().c_str());
        m_dev.set_prop_str("netdev", m_netdev_id.c_str());
    }

    void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);

        regs_socket.init(sbd, 0);
        desc_socket.init(sbd, 1);
        irq_out.init_sbd(sbd, 0);
    }
};
GSC_MODULE_REGISTER(QemuOpencoresEth, sc_core::sc_object*);
#endif //_LIBQBOX_COMPONENTS_OPENCORES_ETH_H
