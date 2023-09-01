/*
 *  This file is part of libqbox
 *  Copyright (c) 2022 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <vector>

#include <cci_configuration>

#include <libqemu-cxx/target/riscv.h>
#include <greensocs/gsutils/module_factory_registery.h>

#include "libqbox/ports/target.h"
#include "libqbox/components/device.h"

class QemuRiscvAclintSwi : public QemuDevice
{
public:
    cci::cci_param<unsigned int> p_num_harts;

    QemuTargetSocket<> socket;

    QemuRiscvAclintSwi(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : QemuRiscvAclintSwi(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    QemuRiscvAclintSwi(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "riscv.aclint.swi")
        , p_num_harts("num_harts", 0, "Number of HARTS this CLINT is connected to")
        , socket("mem", inst)
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_int("num-harts", p_num_harts);
    }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_dev());
        socket.init(qemu::SysBusDevice(m_dev), 0);
    }
};

GSC_MODULE_REGISTER(QemuRiscvAclintSwi, sc_core::sc_object*, sc_core::sc_object*);