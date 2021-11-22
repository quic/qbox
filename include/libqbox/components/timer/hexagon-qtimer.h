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
protected:
    class QemuHexagonHextimer : public QemuDevice {
    public:
        QemuTargetSocket<> &socket;
        QemuInitiatorSignalSocket &irq;

        QemuHexagonHextimer(sc_core::sc_module_name nm, QemuInstance &inst,
                        QemuTargetSocket<> &socket,
                        QemuInitiatorSignalSocket &irq)
            : QemuDevice(nm, inst, "hextimer")
            , socket(socket)
            , irq(irq)
        {}

        void before_end_of_elaboration() override
        {
            /* NO-OP: see Qtimer before_end_of_elaboration */
        }

        void do_before_end_of_elaboration()
        {
            QemuDevice::before_end_of_elaboration();
        }

        void end_of_elaboration() override
        {
            QemuDevice::end_of_elaboration();

            qemu::SysBusDevice sbd(m_dev);
            socket.init(sbd, 0);

            irq.init_sbd(sbd, 0);
        }
    };

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

protected:
    QemuHexagonHextimer timer[2];

public:
    QemuHexagonQtimer(sc_core::sc_module_name nm, QemuInstance &inst)
        : QemuDevice(nm, inst, "qutimer")
        , socket("mem", inst)
        , timer_socket{{"timer0_mem", inst},{"timer1_mem", inst}}
        , timer_irq{{"timer0_irq"},{"timer1_irq"}}
        , timer{{"timer0", inst, timer_socket[0], timer_irq[0]},
                {"timer1", inst, timer_socket[1], timer_irq[1]}}
    {}

    void before_end_of_elaboration() override
    {
        /*
         * FIXME:
         * Due to qemu using a global variable to store the Qtimer
         * we need to ensure we first created the QuTimer
         * Qemu also uses the creation order to set a devid to hextimer
         * so we need to create timer[0] then [1]
         */
        QemuDevice::before_end_of_elaboration();
        timer[0].do_before_end_of_elaboration();
        timer[1].do_before_end_of_elaboration();
    }

    void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);
        socket.init(sbd, 0);
    }
};

#endif
