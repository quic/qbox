/*
 * This file is part of libqbox
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_ARM_GICV3_ITS_H
#define _LIBQBOX_COMPONENTS_ARM_GICV3_ITS_H

#include <module_factory_registery.h>
#include "arm_gicv3.h"
#include <string>
#include <sstream>

class arm_gicv3_its : public QemuDevice
{
public:
    QemuTargetSocket<> mem;

private:
    arm_gicv3* m_parent_gicv3;

public:
    arm_gicv3_its(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : arm_gicv3_its(name, *(dynamic_cast<QemuInstance*>(o)), dynamic_cast<arm_gicv3*>(t))
    {
    }
    arm_gicv3_its(const sc_core::sc_module_name& n, QemuInstance& inst, arm_gicv3* _arm_gicv3)
        : QemuDevice(n, inst, "arm-gicv3-its"), m_parent_gicv3(_arm_gicv3), mem("mem", inst)

    {
    }

    void before_end_of_elaboration()
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_link("parent-gicv3", m_parent_gicv3->get_qemu_dev());
    }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();
        qemu::SysBusDevice sbd(m_dev);
        mem.init(sbd, 0);
    }
};

extern "C" void module_register();

#endif