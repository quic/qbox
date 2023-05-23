/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
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

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H
#define _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H

#include <cci_configuration>

#include <greensocs/libgssync.h>

#include "gpex.h"

class QemuVirtioGpuGlPci : public QemuGPEX::Device
{
public:
    cci::cci_param<uint64_t> p_hostmem_mb;

    QemuVirtioGpuGlPci(const sc_core::sc_module_name& name, QemuInstance& inst)
        : QemuGPEX::Device(name, inst, "virtio-gpu-gl-pci")
        , p_hostmem_mb("hostmem_mb", 2048, "MB to allocate for host visible shared memory") {
#ifndef __APPLE__
        // Use QEMU's integrated display only if we are NOT on MacOS.
        // On MacOS use libqbox's QemuDisplay SystemC module.
        m_inst.set_display_arg("sdl,gl=on");

        m_inst.add_arg("-object");
        auto memory_object = "memory-backend-memfd,id=mem1,size=" +
                             std::to_string(p_hostmem_mb.get_value()) + "M";
        m_inst.add_arg(memory_object.c_str());
        m_inst.add_arg("-machine");
        m_inst.add_arg("memory-backend=mem1");
#endif
    }

    void before_end_of_elaboration() override {
        QemuGPEX::Device::before_end_of_elaboration();

#ifndef __APPLE__
        get_qemu_dev().set_prop_bool("blob", true);
        get_qemu_dev().set_prop_int("hostmem", p_hostmem_mb);
        get_qemu_dev().set_prop_bool("context_init", true);
#endif
    }

    void gpex_realize(qemu::Bus& bus) override { QemuGPEX::Device::gpex_realize(bus); }
};
#endif
