/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_KEYBOARD_PCI_H
#define _LIBQBOX_COMPONENTS_VIRTIO_KEYBOARD_PCI_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "gpex.h"

/// @brief This class wraps the qemu's Virt-IO Keyboard PCI.
class QemuVirtioKeyboardPci : public QemuGPEX::Device
{
public:
    QemuVirtioKeyboardPci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : QemuVirtioKeyboardPci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<QemuGPEX*>(t)))
    {
    }

    QemuVirtioKeyboardPci(const sc_core::sc_module_name& n, QemuInstance& inst, QemuGPEX* gpex)
        : QemuGPEX::Device(n, inst, "virtio-keyboard-pci")
    {
        gpex->add_device(*this);
    }
};

GSC_MODULE_REGISTER(QemuVirtioKeyboardPci, sc_core::sc_object*, sc_core::sc_object*);

#endif // _LIBQBOX_COMPONENTS_VIRTIO_KEYBOARD_PCI_H
