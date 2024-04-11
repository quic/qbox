/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <virtio_gpu_cl_pci.h>

#include <libqemu/config-host.h>

virtio_gpu_cl_pci::virtio_gpu_cl_pci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
    : virtio_gpu_cl_pci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
{
}
virtio_gpu_cl_pci::virtio_gpu_cl_pci(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex)
    : QemuVirtioGpu(name, inst, "cl-pci", gpex)
    , p_hostmem_mb("hostmem_mb", 2048, "MB to allocate for host visible shared memory")
{
#ifndef __APPLE__
#ifdef HAVE_VIRGL_RESOURCE_BLOB
    // Create memory object for host visible shared memory only if blob
    // resources are available in virgl otherwise it is not needed.
    // Also we can NOT enable it on MacOS as it does not have memfd support.
    m_inst.add_arg("-object");
    auto memory_object = "memory-backend-memfd,id=mem1,size=" + std::to_string(p_hostmem_mb.get_value()) + "M";
    m_inst.add_arg(memory_object.c_str());
    m_inst.add_arg("-machine");
    m_inst.add_arg("memory-backend=mem1");
#endif // HAVE_VIRGL_RESOURCE_BLOB
#endif // NOT __APPLE__
}

void virtio_gpu_cl_pci::before_end_of_elaboration()
{
    QemuVirtioGpu::before_end_of_elaboration();

#ifndef __APPLE__
#ifdef HAVE_VIRGL_RESOURCE_BLOB
    // Enable blob resources and host visible shared memory only if
    // available in virgl and we are NOT on MacOS as it does not have
    // /dev/udmabuf support.
    get_qemu_dev().set_prop_bool("blob", true);
    get_qemu_dev().set_prop_int("hostmem", p_hostmem_mb);
#endif // HAVE_VIRGL_RESOURCE_BLOB

    get_qemu_dev().set_prop_bool("context_init", true);
#endif // NOT __APPLE__
}

void module_register()
{
    GSC_MODULE_REGISTER_C(virtio_gpu_cl_pci, sc_core::sc_object*, sc_core::sc_object*);
}