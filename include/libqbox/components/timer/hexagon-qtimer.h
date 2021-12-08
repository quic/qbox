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
    QemuTargetSocket<> timer_socket[2];
    QemuInitiatorSignalSocket timer_irq[2];

public:
    QemuHexagonQtimer(sc_core::sc_module_name nm, QemuInstance &inst)
        : QemuDevice(nm, inst, "qct-qtimer")
        , socket("mem", inst)
        , timer_socket{{"timer0_mem", inst},{"timer1_mem", inst}}
        , timer_irq{{"timer0_irq"},{"timer1_irq"}}
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

        for (int i = 0; i < 2; i++) {
            char buffer[16];
            std::snprintf(buffer, 15, "timer[%d]", i);
            qemu::Object obj = sbd.get_prop_link(buffer);
            qemu::SysBusDevice tim_sbd(obj);

            timer_socket[i].init(tim_sbd, 0);
            timer_irq[i].init_sbd(tim_sbd, 0);
        }
    }
};

#endif
