/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Damien Hedde
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

#ifndef _LIBQBOX_COMPONENTS_TIMER_HEXAGON_QTIMER_H
#define _LIBQBOX_COMPONENTS_TIMER_HEXAGON_QTIMER_H

#include <string>
#include <cassert>

#include "libqbox/ports/target.h"
#include "libqbox/ports/target-signal-socket.h"
#include "libqbox/ports/initiator-signal-socket.h"

class QemuHexagonQtimer : public QemuDevice {
public:
    QemuTargetSocket<> socket;
    /*
     * FIXME:
     * the irq is not really declared as a gpio in qemu, so we cannot
     * import it
     *QemuInitiatorSignalSocket irq;
     */

    /* timers mem/irq */
    QemuTargetSocket<> timer0_socket;
    QemuTargetSocket<> timer1_socket;
    QemuTargetSocket<> timer2_socket;
    QemuInitiatorSignalSocket timer0_irq;
    QemuInitiatorSignalSocket timer1_irq;
    QemuInitiatorSignalSocket timer2_irq;

public:
    QemuHexagonQtimer(sc_core::sc_module_name nm, QemuInstance &inst)
        : QemuDevice(nm, inst, "qct-qtimer")
        , socket("mem", inst)
        , timer0_socket("timer0_mem", inst)
        , timer1_socket("timer1_mem", inst)
        , timer2_socket("timer2_mem", inst)
        , timer0_irq("timer0_irq")
        , timer1_irq("timer1_irq")
        , timer2_irq("timer2_irq")
    {}

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();
    }

    void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);
        socket.init(sbd, 0);

        char buffer[16];
        std::snprintf(buffer, 15, "timer[0]");
        qemu::Object obj = sbd.get_prop_link(buffer);
        qemu::SysBusDevice tim_sbd(obj);
        timer0_socket.init(tim_sbd, 0);
        timer0_irq.init_sbd(tim_sbd, 0);

        std::snprintf(buffer, 15, "timer[1]");
        obj = sbd.get_prop_link(buffer);
        qemu::SysBusDevice tim_sbd1(obj);
        timer1_socket.init(tim_sbd1, 0);
        timer1_irq.init_sbd(tim_sbd1, 0);

        std::snprintf(buffer, 15, "timer[2]");
        obj = sbd.get_prop_link(buffer);
        qemu::SysBusDevice tim_sbd2(obj);
        timer2_socket.init(tim_sbd2, 0);
        timer2_irq.init_sbd(tim_sbd2, 0);
    }
};

#endif
