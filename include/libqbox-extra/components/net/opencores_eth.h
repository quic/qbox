/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *  Author: Lukas Jünger
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

#include "libqbox/components/device.h"
#include "libqbox/ports/target.h"
#include "libqbox/ports/initiator-signal-socket.h"

class QemuOpencoresEth : public QemuDevice {
protected:
    cci::cci_param<std::string> p_mac;
    cci::cci_param<std::string> p_netdev_id;

public:
    QemuTargetSocket<> regs_socket;
    QemuTargetSocket<> desc_socket;
    QemuInitiatorSignalSocket irq_out;

    QemuOpencoresEth(const sc_core::sc_module_name &n, QemuInstance &inst)
        : QemuDevice(n, inst, "open_eth")
        , p_mac("mac", "00:11:22:33:44:55", "MAC address of NIC")
        , p_netdev_id("netdev-id", "gs_net_id", "netdev id of NIC to select netdev backend")
        , regs_socket("regs", inst)
        , desc_socket("desc", inst)
        , irq_out("irq-out")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_str("mac", p_mac.get_value().c_str());
        m_dev.set_prop_str("netdev", p_netdev_id.get_value().c_str());
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

#endif //_LIBQBOX_COMPONENTS_OPENCORES_ETH_H
