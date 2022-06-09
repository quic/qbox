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

#ifndef _LIBQBOX_COMPONENTS_UART_PL011_H
#define _LIBQBOX_COMPONENTS_UART_PL011_H

#include <cci_configuration>

#include <greensocs/libgssync.h>

#include "libqbox/components/device.h"
#include "libqbox/ports/target.h"
#include "libqbox/ports/initiator-signal-socket.h"

class QemuUartPl011 : public QemuDevice {
protected:
    qemu::Chardev m_chardev;

    /* FIXME: temp, should be in the backend */
    gs::async_event m_ext_ev;

public:
    QemuTargetSocket<> socket;
    QemuInitiatorSignalSocket irq_out;

    QemuUartPl011(const sc_core::sc_module_name &n, QemuInstance &inst)
        : QemuDevice(n, inst, "pl011")
        , m_ext_ev(true)
        , socket("mem", inst)
        , irq_out("irq_out")
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

#endif
