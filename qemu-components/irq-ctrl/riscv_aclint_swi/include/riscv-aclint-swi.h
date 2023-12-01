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
#include <module_factory_registery.h>

#include <ports/target.h>
#include <device.h>

class riscv_aclint_swi : public QemuDevice
{
public:
    cci::cci_param<unsigned int> p_num_harts;

    QemuTargetSocket<> socket;

    riscv_aclint_swi(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : riscv_aclint_swi(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    riscv_aclint_swi(sc_core::sc_module_name nm, QemuInstance& inst)
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

extern "C" void module_register();
