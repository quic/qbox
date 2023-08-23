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

#ifndef _LIBQBOX_COMPONENTS_UART_16550_H
#define _LIBQBOX_COMPONENTS_UART_16550_H

#include <cci_configuration>

#include <greensocs/gsutils/module_factory_registery.h>

#include "libqbox/components/device.h"
#include "libqbox/ports/target.h"
#include "libqbox/ports/initiator-signal-socket.h"

class QemuUart16550 : public QemuDevice
{
protected:
    qemu::Chardev m_chardev;

    cci::cci_param<unsigned int> p_baudbase;
    cci::cci_param<unsigned int> p_regshift;

public:
    QemuTargetSocket<> socket;
    QemuInitiatorSignalSocket irq_out;

    QemuUart16550(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : QemuUart16550(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    QemuUart16550(const sc_core::sc_module_name& n, QemuInstance& inst)
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
GSC_MODULE_REGISTER(QemuUart16550, sc_core::sc_object*);
#endif
