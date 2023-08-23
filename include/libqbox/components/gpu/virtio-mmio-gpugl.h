/*
 *  This file is part of libqbox
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

#pragma once

#include <vector>

#include <cci_configuration>

#include "libqbox/components/device.h"
#include "libqbox/ports/target.h"
#include "libqbox/ports/initiator-signal-socket.h"
#include "libqbox/components/virtio/virtio-mmio.h"

class QemuVirtioMMIOGpuGl : public QemuVirtioMMIO
{
private:
public:
    QemuVirtioMMIOGpuGl(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuVirtioMMIO(nm, inst, "virtio-gpu-gl-device")
    {
#ifndef __APPLE__
        // Use QEMU's integrated display only if we are NOT on MacOS.
        // On MacOS use libqbox's QemuDisplay SystemC module.
        m_inst.set_display_arg("sdl,gl=on");
#endif
    }

    void before_end_of_elaboration() override { QemuVirtioMMIO::before_end_of_elaboration(); }
};
