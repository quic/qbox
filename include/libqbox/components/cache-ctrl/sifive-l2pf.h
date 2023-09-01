/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_CACHE_CTRL_SIFIVE_L2PF_H
#define _LIBQBOX_COMPONENTS_CACHE_CTRL_SIFIVE_L2PF_H

#include <string>
#include <cassert>

#include <cci_configuration>

#include "libqbox/ports/target.h"
#include "libqbox/ports/target-signal-socket.h"

class QemuRiscvSifiveL2pf : public QemuDevice
{
public:
    QemuTargetSocket<> socket;

    QemuRiscvSifiveL2pf(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "sifive.l2pf"), socket("mem", inst)
    {
    }

    void before_end_of_elaboration() override { QemuDevice::before_end_of_elaboration(); }

    void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_dev());
        socket.init(qemu::SysBusDevice(m_dev), 0);
    }
};

#endif
