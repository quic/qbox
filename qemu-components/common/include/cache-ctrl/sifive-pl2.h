/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_CACHE_CTRL_SIFIVE_PL2_H
#define _LIBQBOX_COMPONENTS_CACHE_CTRL_SIFIVE_PL2_H

#include <string>
#include <cassert>

#include <cci_configuration>

#include <ports/target.h>
#include <ports/qemu-target-signal-socket.h>

class QemuRiscvSifivePl2 : public QemuDevice
{
public:
    QemuTargetSocket<> socket;

    QemuRiscvSifivePl2(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "sifive,pL2Cache0"), socket("mem", inst)
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
