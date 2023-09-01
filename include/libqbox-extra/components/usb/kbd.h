/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_USB_KBD_H
#define _LIBQBOX_COMPONENTS_USB_KBD_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "libqbox-extra/components/pci/xhci.h"

class QemuKbd : public QemuXhci::Device
{
public:
    QemuKbd(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : QemuKbd(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<QemuXhci*>(t)))
    {
    }
    QemuKbd(const sc_core::sc_module_name& n, QemuInstance& inst, QemuXhci* xhci): QemuXhci::Device(n, inst, "usb-kbd")
    {
        xhci->add_device(*this);
    }
};
GSC_MODULE_REGISTER(QemuKbd, sc_core::sc_object*, sc_core::sc_object*);
#endif
