/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_GPU_PCI_H
#define _LIBQBOX_COMPONENTS_VIRTIO_GPU_PCI_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "virtio-gpu.h"

class QemuVirtioGpuPci : public QemuVirtioGpu
{
public:
    QemuVirtioGpuPci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : QemuVirtioGpuPci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<QemuGPEX*>(t)))
    {
    }
    QemuVirtioGpuPci(const sc_core::sc_module_name& name, QemuInstance& inst, QemuGPEX* gpex): QemuVirtioGpu(name, inst, "pci", gpex) {}
};
GSC_MODULE_REGISTER(QemuVirtioGpuPci, sc_core::sc_object*, sc_core::sc_object*);
#endif
