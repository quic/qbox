/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H
#define _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H

#include <module_factory_registery.h>

#include <virtio_gpu.h>

class virtio_gpu_gl_pci : public QemuVirtioGpu
{
public:
    cci::cci_param<uint64_t> p_hostmem_mb;

    virtio_gpu_gl_pci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t);
    virtio_gpu_gl_pci(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex);

    void before_end_of_elaboration() override;
};

extern "C" void module_register();
#endif // _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H
