/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H
#define _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H

#include <greensocs/gsutils/module_factory_registery.h>

#include "virtio-gpu.h"

class QemuVirtioGpuGlPci : public QemuVirtioGpu
{
public:
    cci::cci_param<uint64_t> p_hostmem_mb;

    QemuVirtioGpuGlPci(const sc_core::sc_module_name& name, sc_core::sc_object* o);
    QemuVirtioGpuGlPci(const sc_core::sc_module_name& name, QemuInstance& inst);

    void before_end_of_elaboration() override;
};
GSC_MODULE_REGISTER(QemuVirtioGpuGlPci, sc_core::sc_object*);
#endif // _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H
