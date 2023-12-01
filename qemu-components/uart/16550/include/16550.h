/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_UART_16550_H
#define _LIBQBOX_COMPONENTS_UART_16550_H

#include <cci_configuration>

#include <module_factory_registery.h>

#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>

class uart_16550 : public QemuDevice
{
protected:
    qemu::Chardev m_chardev;

    cci::cci_param<unsigned int> p_baudbase;
    cci::cci_param<unsigned int> p_regshift;

public:
    QemuTargetSocket<> socket;
    QemuInitiatorSignalSocket irq_out;

    uart_16550(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : uart_16550(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    uart_16550(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "serial-mm")
        , p_baudbase("baudbase", 38400000,
                     "Base frequency from which the baudrate "
                     "is derived (in Hz)")
        , p_regshift("regshift", 2,
                     "Shift to apply to the MMIO register map "
                     "(2 means one reg = 32 bits)")
        , socket("mem", inst)
        , irq_out("irq_out")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_int("baudbase", p_baudbase);
        m_dev.set_prop_int("regshift", p_regshift);

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
