/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc.
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

#include "libqbox-extra/components/pci/virtio-gpu-gl-pci.h"

#include <libqemu/config-host.h>

QemuVirtioGpuGlPci::QemuVirtioGpuGlPci(const sc_core::sc_module_name& name, sc_core::sc_object* o)
    : QemuVirtioGpuGlPci(name, *(dynamic_cast<QemuInstance*>(o)))
{
}
QemuVirtioGpuGlPci::QemuVirtioGpuGlPci(const sc_core::sc_module_name& name, QemuInstance& inst)
    : QemuVirtioGpu(name, inst, "gl-pci")
    , p_hostmem_mb("hostmem_mb", 2048, "MB to allocate for host visible shared memory")
{
#ifndef __APPLE__
    // Use QEMU's integrated display only if we are NOT on MacOS.
    // On MacOS use libqbox's QemuDisplay SystemC module.
    m_inst.set_display_arg("sdl,gl=on");

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

void QemuVirtioGpuGlPci::before_end_of_elaboration()
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
