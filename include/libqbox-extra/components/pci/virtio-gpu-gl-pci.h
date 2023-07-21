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

#include "virtio-gpu.h"

class QemuVirtioGpuGlPci : public QemuVirtioGpu
{
public:
    cci::cci_param<uint64_t> p_hostmem_mb;

    QemuVirtioGpuGlPci(const sc_core::sc_module_name& name, QemuInstance& inst);

    void before_end_of_elaboration() override;
};

#endif // _LIBQBOX_COMPONENTS_VIRTIO_GPU_GL_PCI_H
