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

#ifndef _LIBQBOX_COMPONENTS_VIRTIO_GPU_H
#define _LIBQBOX_COMPONENTS_VIRTIO_GPU_H

#include <cci_configuration>

#include <greensocs/libgssync.h>

#include "gpex.h"

class QemuVirtioGpu : public QemuGPEX::Device
{
public:
    void before_end_of_elaboration() override { QemuGPEX::Device::before_end_of_elaboration(); }

    void gpex_realize(qemu::Bus& bus) override { QemuGPEX::Device::gpex_realize(bus); }

protected:
    QemuVirtioGpu(const sc_core::sc_module_name& name, QemuInstance& inst, const char* gpu_type)
        : QemuGPEX::Device(name, inst, (std::string("virtio-gpu-") + gpu_type).c_str())
    {
    }
};
#endif
