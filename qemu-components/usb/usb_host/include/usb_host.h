/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_USB_HOST_H
#define _LIBQBOX_COMPONENTS_USB_HOST_H

#include <module_factory_registery.h>
#include "qemu_xhci.h"
#include <string>

class usb_host : public qemu_xhci::Device
{
protected:
    cci::cci_param<uint32_t> p_hostbus;
    cci::cci_param<uint32_t> p_hostaddr;
    cci::cci_param<std::string> p_hostport;
    cci::cci_param<uint32_t> p_vendorid;
    cci::cci_param<uint32_t> p_productid;

    cci::cci_param<uint32_t> p_isobufs;
    cci::cci_param<uint32_t> p_isobsize;
    cci::cci_param<bool> p_guest_reset;
    cci::cci_param<bool> p_guest_resets_all;
    cci::cci_param<bool> p_suppress_remote_wake;

public:
    usb_host(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : usb_host(name, *(dynamic_cast<QemuInstance*>(o)), dynamic_cast<qemu_xhci*>(t))
    {
    }
    usb_host(const sc_core::sc_module_name& n, QemuInstance& inst, qemu_xhci* xhci)
        : qemu_xhci::Device(n, inst, "usb-host")
        , p_hostbus("hostbus", 0, "host usb device bus")
        , p_hostaddr("hostaddr", 0, "host usb device addr")
        , p_hostport("hostport", std::string(""), "host usb device port")
        , p_vendorid("vendorid", 0, "host usb device vendorid")
        , p_productid("productid", 0, "host usb device productid")
        , p_isobufs("isobufs", 4, "usb isobufs")
        , p_isobsize("isobsize", 32, "usb isobsize")
        , p_guest_reset("guest_reset", true, "guest usb reset")
        , p_guest_resets_all("guest_resets_all", false, "guest usb resets all")
        , p_suppress_remote_wake("suppress_remote_wake", true, "suppress remote wake")
    {
        set_dev_props = [this]() -> void {
            m_dev.set_prop_uint("hostbus", p_hostbus.get_value());
            m_dev.set_prop_uint("hostaddr", p_hostaddr.get_value());
            if (!p_hostport.get_value().empty()) {
                m_dev.set_prop_str("hostport", p_hostport.get_value().c_str());
            }
            m_dev.set_prop_uint("vendorid", p_vendorid.get_value());
            m_dev.set_prop_uint("productid", p_productid.get_value());
            m_dev.set_prop_uint("isobufs", p_isobufs.get_value());
            m_dev.set_prop_uint("isobsize", p_isobsize.get_value());
            m_dev.set_prop_bool("guest-reset", p_guest_reset.get_value());
            m_dev.set_prop_bool("guest-resets-all", p_guest_resets_all.get_value());
            m_dev.set_prop_bool("suppress-remote-wake", p_suppress_remote_wake.get_value());
        };

        xhci->add_device(*this);
    }
};

extern "C" void module_register();

#endif
