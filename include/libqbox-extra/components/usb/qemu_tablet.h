/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_USB_TABLET_H
#define _LIBQBOX_COMPONENTS_USB_TABLET_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "libqbox-extra/components/pci/qemu_xhci.h"

/// @brief Tablet touch device (mouse)
class qemu_tablet : public qemu_xhci::Device
{
public:
    qemu_tablet(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : qemu_tablet(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_xhci*>(t)))
    {
    }
    qemu_tablet(const sc_core::sc_module_name& n, QemuInstance& inst, qemu_xhci* xhci)
        : qemu_xhci::Device(n, inst, "usb-tablet")
    {
        xhci->add_device(*this);
    }
};

extern "C" void module_register();
#endif
