/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_USB_TABLET_H
#define _LIBQBOX_COMPONENTS_USB_TABLET_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "libqbox-extra/components/pci/xhci.h"

/// @brief Tablet touch device (mouse)
class QemuTablet : public QemuXhci::Device
{
public:
    QemuTablet(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : QemuTablet(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<QemuXhci*>(t)))
    {
    }
    QemuTablet(const sc_core::sc_module_name& n, QemuInstance& inst, QemuXhci* xhci)
        : QemuXhci::Device(n, inst, "usb-tablet")
    {
        xhci->add_device(*this);
    }
};
GSC_MODULE_REGISTER(QemuTablet, sc_core::sc_object*, sc_core::sc_object*);
#endif
