/*
 * This file is part of libqbox
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_PL031_H
#define _LIBQBOX_COMPONENTS_PL031_H

#include <systemc>
#include <cci_configuration>
#include <module_factory_registery.h>
#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <tlm_sockets_buswidth.h>
#include <vector>

class pl031 : public QemuDevice
{
public:
    QemuTargetSocket<> q_socket;
    QemuInitiatorSignalSocket irq_out;

public:
    pl031(const sc_core::sc_module_name& name, sc_core::sc_object* o): pl031(name, *(dynamic_cast<QemuInstance*>(o))) {}
    pl031(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "pl031"), q_socket("mem", inst), irq_out("irq_out")
    {
    }

    void before_end_of_elaboration() override { QemuDevice::before_end_of_elaboration(); }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();
        qemu::SysBusDevice sbd(m_dev);
        q_socket.init(sbd, 0);
        irq_out.init_sbd(sbd, 0);
    }
};

extern "C" void module_register();

#endif
