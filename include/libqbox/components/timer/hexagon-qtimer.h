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
#include "libqbox/sc-qemu-instance.h"

class QemuHexagonQtimer : public QemuDevice
{
protected:
    cci::cci_param<unsigned int> p_nr_frames;
    cci::cci_param<unsigned int> p_nr_views;
    cci::cci_param<unsigned int> p_cnttid;

public:
    QemuTargetSocket<> socket;

    /*
     * FIXME:
     * the irq is not really declared as a gpio in qemu, so we cannot
     * import it
     *QemuInitiatorSignalSocket irq;
     */

    /* timers mem/irq */
    QemuTargetSocket<> view_socket;
    sc_core::sc_vector<QemuInitiatorSignalSocket> irq;

public:
    QemuHexagonQtimer(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : QemuHexagonQtimer(name, dynamic_cast<SC_QemuInstance*>(o)->getQemuInst())
        {
        }
    QemuHexagonQtimer(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "qct-qtimer")
        , p_nr_frames("nr_frames", 2, "Number of frames")
        , p_nr_views("nr_views", 1, "Number of views")
        , p_cnttid("cnttid", 0x11, "Value of cnttid")
        , socket("mem", inst)
        , view_socket("mem_view", inst)
        , irq("irq", p_nr_frames.get_value(),
              [](const char* n, size_t i) { return new QemuInitiatorSignalSocket(n); }) {}

    void before_end_of_elaboration() override {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_int("nr_frames", p_nr_frames);
        m_dev.set_prop_int("nr_views", p_nr_views);
        m_dev.set_prop_int("cnttid", p_cnttid);
    }

    void end_of_elaboration() override {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);
        socket.init(sbd, 0);
        view_socket.init(sbd, 1);

        for (uint32_t i = 0; i < p_nr_frames.get_value(); ++i) {
            irq[i].init_sbd(sbd, i);
        }
    }
};

#endif
