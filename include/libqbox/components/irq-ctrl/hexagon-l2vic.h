/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
    QemuTargetSignalSocket reset;

    QemuHexagonL2vic(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "l2vic")
        , p_num_sources(1024) /* this is hardcoded in qemu */
        , p_num_outputs(8)    /* this is hardcoded in qemu */
        , socket("mem", inst)
        , socket_fast("fastmem", inst)
        , irq_in("irq_in", p_num_sources, [](const char* n, int i) { return new QemuTargetSignalSocket(n); })
        , irq_out("irq_out", p_num_outputs, [](const char* n, int i) { return new QemuInitiatorSignalSocket(n); })
        , reset("reset")
    {
    }

    void before_end_of_elaboration() override { QemuDevice::before_end_of_elaboration(); }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);
        socket.init(sbd, 0);
        socket_fast.init(sbd, 1);

        for (int i = 0; i < p_num_sources; i++) {
            irq_in[i].init(m_dev, i);
        }
        reset.init_named(m_dev, "reset", 0);
        for (int i = 0; i < p_num_outputs; i++) {
            irq_out[i].init_sbd(sbd, i);
        }
    }
};

#endif
