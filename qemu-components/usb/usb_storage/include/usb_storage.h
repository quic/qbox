/*
 * This file is part of libqbox
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_USB_STORAGE_H
#define _LIBQBOX_COMPONENTS_USB_STORAGE_H

#include <module_factory_registery.h>

#include <qemu_xhci.h>

class usb_storage : public qemu_xhci::Device
{
private:
    std::string blkdev_id;
    cci::cci_param<std::string> blkdev_str;

public:
    usb_storage(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : usb_storage(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_xhci*>(t)))
    {
    }
    usb_storage(const sc_core::sc_module_name& n, QemuInstance& inst, qemu_xhci* xhci)
        : qemu_xhci::Device(n, inst, "usb-storage")
        , blkdev_id(std::string(name()) + "-id")
        , blkdev_str("blkdev_str", "", "blkdev string for QEMU (do not specify ID)")
    {
        if (!blkdev_str.get_value().empty()) {
            std::stringstream opts;
            opts << blkdev_str.get_value();
            opts << ",id=" << blkdev_id;

            m_inst.add_arg("-drive");
            m_inst.add_arg(opts.str().c_str());

            set_dev_props = [this]() -> void { m_dev.set_prop_parse("drive", blkdev_id.c_str()); };
        } else {
            SCP_FATAL(()) << "usb storage needs a blkdev_str CCI Parameter!";
        }
        xhci->add_device(*this);
    }
};

extern "C" void module_register();
#endif
