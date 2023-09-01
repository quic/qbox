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

class QemuRiscvAclintMtimer : public QemuDevice
{
public:
    cci::cci_param<unsigned int> p_num_harts;
    cci::cci_param<uint64_t> p_timecmp_base;
    cci::cci_param<uint64_t> p_time_base;
    cci::cci_param<uint64_t> p_aperture_size;
    cci::cci_param<uint32_t> p_timebase_freq;
    cci::cci_param<bool> p_provide_rdtime;

    QemuTargetSocket<> socket;

    QemuRiscvAclintMtimer(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : QemuRiscvAclintMtimer(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    QemuRiscvAclintMtimer(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "riscv.aclint.mtimer")
        , p_num_harts("num_harts", 0, "Number of HARTS this CLINT is connected to")
        , p_timecmp_base("timecmp_base", 0, "Base address for the TIMECMP registers")
        , p_time_base("time_base", 0, "Base address for the TIME registers")
        , p_aperture_size("aperture_size", 0, "Size of the whole CLINT address space")
        , p_timebase_freq("timebase_freq", 10000000, "")
        , p_provide_rdtime("provide_rdtime", false,
                           "If true, provide the CPU with "
                           "a rdtime register")
        , socket("mem", inst)
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_int("num-harts", p_num_harts);
        m_dev.set_prop_int("timecmp-base", p_timecmp_base);
        m_dev.set_prop_int("time-base", p_time_base);
        m_dev.set_prop_int("aperture-size", p_aperture_size);
        m_dev.set_prop_int("timebase-freq", p_timebase_freq);
        m_dev.set_prop_bool("provide-rdtime", p_provide_rdtime);
    }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_dev());
        socket.init(qemu::SysBusDevice(m_dev), 0);
    }
};

GSC_MODULE_REGISTER(QemuRiscvAclintMtimer, sc_core::sc_object*);