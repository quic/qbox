/*
 * This file is part of libqbox
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_USB_XHCI_H
#define _LIBQBOX_COMPONENTS_USB_XHCI_H

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <module_factory_registery.h>

#include <qemu_gpex.h>

/// @brief This class wraps the qemu's XHCI USB controller: eXtensible Host Controller Interface.
class qemu_xhci : public qemu_gpex::Device
{
public:
    class Device : public QemuDevice
    {
        friend qemu_xhci;

    public:
        Device(const sc_core::sc_module_name& name, QemuInstance& inst, const char* qom_type)
            : QemuDevice(name, inst, qom_type)
        {
        }

        /// @brief We cannot do the end_of_elaboration at this point because we
        /// need the USB bus (created only during the USB host realize step)
        void end_of_elaboration() override {}

    protected:
        virtual void usb_realize(qemu::Bus& usb_bus)
        {
            m_dev.set_parent_bus(usb_bus);
            QemuDevice::end_of_elaboration();
        }
    };

protected:
    std::vector<Device*> m_devices;

public:
    qemu_xhci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : qemu_xhci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
    {
    }
    qemu_xhci(const sc_core::sc_module_name& n, QemuInstance& inst, qemu_gpex* gpex)
        : qemu_gpex::Device(n, inst, "qemu-xhci")
    {
        gpex->add_device(*this);
    }

    void add_device(Device& dev)
    {
        if (m_inst != dev.get_qemu_inst()) {
            SCP_FATAL(SCMOD) << "USB device and host have to be in same qemu instance";
        }
        m_devices.push_back(&dev);
    }

    void end_of_elaboration() override
    {
        qemu_gpex::Device::end_of_elaboration();

        qemu::Bus root_bus = m_dev.get_child_bus("usb-bus.0");
        for (Device* device : m_devices) {
            device->usb_realize(root_bus);
        }
    }
};

extern "C" void module_register();
#endif
