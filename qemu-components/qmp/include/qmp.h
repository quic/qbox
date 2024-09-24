/*
 * This file is part of libqbox
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_RESET_QMP_H
#define _LIBQBOX_COMPONENTS_RESET_QMP_H

#include <ports/qemu-initiator-signal-socket.h>
#include <ports/qemu-target-signal-socket.h>
#include <device.h>
#include <qemu-instance.h>
#include <module_factory_registery.h>

class qmp : public sc_core::sc_module
{
public:
    SCP_LOGGER();

    cci::cci_param<std::string> p_qmp_str;

    qmp(const sc_core::sc_module_name& name, sc_core::sc_object* o): qmp(name, *(dynamic_cast<QemuInstance*>(o))) {}
    qmp(const sc_core::sc_module_name& n, QemuInstance& inst)
        : sc_core::sc_module(n), p_qmp_str("qmp_str", "", "qmp options string, i.e. unix:./qmp-sock,server,wait=off")
    {
        SCP_TRACE(())("qmp constructor");
        if (p_qmp_str.get_value().empty()) {
            SCP_FATAL(())("qmp options string is empty!");
        }
        // https://qemu.weilnetz.de/doc/2.10/qemu-qmp-ref.html
        inst.add_arg("-qmp");
        inst.add_arg(p_qmp_str.get_value().c_str());
    }
};

extern "C" void module_register();
#endif //_LIBQBOX_COMPONENTS_QMP_H
