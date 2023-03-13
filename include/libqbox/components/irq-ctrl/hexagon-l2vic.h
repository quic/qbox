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

#ifndef _LIBQBOX_COMPONENTS_IRQ_CTRL_HEXAGON_L2VIC_H
#define _LIBQBOX_COMPONENTS_IRQ_CTRL_HEXAGON_L2VIC_H

#include <string>
#include <cassert>

#include "libqbox/ports/target.h"
#include "libqbox/ports/target-signal-socket.h"
#include "libqbox/ports/initiator-signal-socket.h"

class QemuHexagonL2vic : public QemuDevice
{
public:
    const unsigned int p_num_sources;
    const unsigned int p_num_outputs;

    QemuTargetSocket<> socket;
    QemuTargetSocket<> socket_fast;
    sc_core::sc_vector<QemuTargetSignalSocket> irq_in;
    sc_core::sc_vector<QemuInitiatorSignalSocket> irq_out;

    QemuHexagonL2vic(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "l2vic")
        , p_num_sources(1024) /* this is hardcoded in qemu */
        , p_num_outputs(8)    /* this is hardcoded in qemu */
        , socket("mem", inst)
        , socket_fast("fastmem", inst)
        , irq_in("irq_in", p_num_sources,
                 [](const char* n, int i) { return new QemuTargetSignalSocket(n); })
        , irq_out("irq_out", p_num_outputs,
                  [](const char* n, int i) { return new QemuInitiatorSignalSocket(n); }) {}

    void before_end_of_elaboration() override { QemuDevice::before_end_of_elaboration(); }

    void end_of_elaboration() override {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);
        socket.init(sbd, 0);
        socket_fast.init(sbd, 1);

        for (int i = 0; i < p_num_sources; i++) {
            irq_in[i].init(m_dev, i);
        }
        for (int i = 0; i < p_num_outputs; i++) {
            irq_out[i].init_sbd(sbd, i);
        }
    }
};

#endif
