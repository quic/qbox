/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_UART_SIFIVE_UART_H
#define _LIBQBOX_COMPONENTS_UART_SIFIVE_UART_H

#include <cci_configuration>

#include <libgssync.h>

#include <module_factory_registery.h>

#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>

class sifive_uart : public QemuDevice
{
protected:
    qemu::Chardev m_chardev;

    /* FIXME: temp, should be in the backend */
    gs::async_event m_ext_ev;

public:
    QemuTargetSocket<> socket;
    QemuInitiatorSignalSocket irq_out;

    sifive_uart(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : sifive_uart(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    sifive_uart(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "riscv.sifive.uart"), m_ext_ev(true), socket("mem", inst), irq_out("irq_out")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        /* FIXME: hardcoded for now */
        m_chardev = m_inst.get().chardev_new("uart", "stdio");
        m_dev.set_prop_chardev("chardev", m_chardev);
    }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);

        socket.init(sbd, 0);
        irq_out.init_sbd(sbd, 0);
    }
};

extern "C" void module_register();

#endif
