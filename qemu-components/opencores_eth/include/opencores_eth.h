/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_OPENCORES_ETH_H
#define _LIBQBOX_COMPONENTS_OPENCORES_ETH_H

#include <module_factory_registery.h>

#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>

class opencores_eth : public QemuDevice
{
protected:
    cci::cci_param<std::string> p_mac;
    std::string m_netdev_id;
    cci::cci_param<std::string> p_netdev_str;

public:
    QemuTargetSocket<> regs_socket;
    QemuTargetSocket<> desc_socket;
    QemuInitiatorSignalSocket irq_out;

    opencores_eth(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : opencores_eth(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    opencores_eth(const sc_core::sc_module_name& n, QemuInstance& inst)
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

extern "C" void module_register();

#endif //_LIBQBOX_COMPONENTS_OPENCORES_ETH_H
