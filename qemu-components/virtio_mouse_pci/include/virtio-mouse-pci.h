/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_MOUSE_PCI_H
#define _LIBQBOX_COMPONENTS_VIRTIO_MOUSE_PCI_H

#include <module_factory_registery.h>

#include <qemu_gpex.h>

/// @brief This class wraps the qemu's Virt-IO Mouse PCI.
class virtio_mouse_pci : public qemu_gpex::Device
{
public:
    virtio_mouse_pci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : virtio_mouse_pci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
    {
    }

    virtio_mouse_pci(const sc_core::sc_module_name& n, QemuInstance& inst, qemu_gpex* gpex)
        : qemu_gpex::Device(n, inst, "virtio-mouse-pci")
    {
        gpex->add_device(*this);
    }
};

extern "C" void module_register();

#endif // _LIBQBOX_COMPONENTS_VIRTIO_MOUSE_PCI_H
