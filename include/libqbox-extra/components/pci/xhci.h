/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LIBQBOX_COMPONENTS_USB_XHCI_H
#define _LIBQBOX_COMPONENTS_USB_XHCI_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "gpex.h"

/// @brief This class wraps the qemu's XHCI USB controller: eXtensible Host Controller Interface.
class QemuXhci : public QemuGPEX::Device
{
public:
    class Device : public QemuDevice
    {
        friend QemuXhci;

    public:
        Device(const sc_core::sc_module_name& name, QemuInstance& inst, const char* qom_type)
            : QemuDevice(name, inst, qom_type) {}

        /// @brief We cannot do the end_of_elaboration at this point because we
        /// need the USB bus (created only during the USB host realize step)
        void end_of_elaboration() override {}

    protected:
        virtual void usb_realize(qemu::Bus& usb_bus) {
            m_dev.set_parent_bus(usb_bus);
            QemuDevice::end_of_elaboration();
        }
    };

protected:
    std::vector<Device*> m_devices;

public:
    QemuXhci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
    : QemuXhci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<QemuGPEX*>(t)))
    {
    }
    QemuXhci(const sc_core::sc_module_name& n, QemuInstance& inst, QemuGPEX* gpex)
        : QemuGPEX::Device(n, inst, "qemu-xhci") 
        {
            gpex->add_device(*this);
        }

    void add_device(Device& dev) {
        if (m_inst != dev.get_qemu_inst()) {
            SCP_FATAL(SCMOD) << "USB device and host have to be in same qemu instance";
        }
        m_devices.push_back(&dev);
    }

    void end_of_elaboration() override {
        QemuGPEX::Device::end_of_elaboration();

        qemu::Bus root_bus = m_dev.get_child_bus("usb-bus.0");
        for (Device* device : m_devices) {
            device->usb_realize(root_bus);
        }
    }
};
GSC_MODULE_REGISTER(QemuXhci, sc_core::sc_object*, sc_core::sc_object*);
#endif
