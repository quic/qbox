/*
 * This file is part of libqbox
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_RAMFB_H
#define _LIBQBOX_COMPONENTS_RAMFB_H

#include <systemc>
#include <tlm>
#include <cci_configuration>
#include <module_factory_registery.h>
#include <qemu-instance.h>
#include <device.h>
#include <libqemu/libqemu.h>
#include <fw_cfg.h>
#include <cstdlib>

class ramfb : public QemuDevice
{
public:
    ramfb(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* f)
        : ramfb(name, *(dynamic_cast<QemuInstance*>(o)), dynamic_cast<fw_cfg*>(f))
    {
    }

    ramfb(const sc_core::sc_module_name& n, QemuInstance& inst, fw_cfg* fc)
        : QemuDevice(n, inst, "ramfb", std::string(n).append(".").append("ramfb").c_str())
    {
        if (!fc) {
            SCP_FATAL(()) << "fw_cfg instance should already exist before ramfb is instantiated!";
        }
    }

    ~ramfb() { unrealize(); }

    void before_end_of_elaboration() override { QemuDevice::before_end_of_elaboration(); }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();
    }
};

extern "C" void module_register();

#endif