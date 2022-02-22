/*
 *  This file is part of libqbox
 *  Copyright (c) 2022 Greensocs
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

class QemuVirtioMMIO: public QemuDevice {
protected:

public:
    QemuTargetSocket<> socket;
    QemuInitiatorSignalSocket irq_out;
    QemuDevice virtio_mmio_device;

    QemuVirtioMMIO(sc_core::sc_module_name nm, QemuInstance &inst, const char* device_type)
            : QemuDevice(nm, inst, device_type)
            , virtio_mmio_device("virtio-mmio", inst, "virtio-mmio")
            , socket("mem", inst)
            , irq_out("irq-out")
    {
    }

    void before_end_of_elaboration()
    {
        virtio_mmio_device.before_end_of_elaboration();
        virtio_mmio_device.get_qemu_dev().set_prop_bool("force-legacy", true);
    }

    void end_of_elaboration()
    {
        virtio_mmio_device.end_of_elaboration();

        qemu::SysBusDevice sbd(virtio_mmio_device.get_qemu_dev());
        socket.init(sbd, 0);
        irq_out.init_sbd(sbd, 0);
    }

    qemu::Bus get_bus() {
        qemu::Device virtio_mmio_dev(virtio_mmio_device.get_qemu_dev());
        return virtio_mmio_dev.get_prop_link("virtio-mmio-bus");
    }
};
