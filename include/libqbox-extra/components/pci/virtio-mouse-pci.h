/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_MOUSE_PCI_H
#define _LIBQBOX_COMPONENTS_VIRTIO_MOUSE_PCI_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "gpex.h"

/// @brief This class wraps the qemu's Virt-IO Mouse PCI.
class QemuVirtioMousePci : public QemuGPEX::Device
{
public:
    QemuVirtioMousePci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : QemuVirtioMousePci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<QemuGPEX*>(t)))
    {
    }

    QemuVirtioMousePci(const sc_core::sc_module_name& n, QemuInstance& inst, QemuGPEX* gpex)
        : QemuGPEX::Device(n, inst, "virtio-mouse-pci")
    {
        gpex->add_device(*this);
    }
};

GSC_MODULE_REGISTER(QemuVirtioMousePci, sc_core::sc_object*, sc_core::sc_object*);

#endif // _LIBQBOX_COMPONENTS_VIRTIO_MOUSE_PCI_H
