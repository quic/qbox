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
#include "libqbox/components/virtio/virtio-mmio.h"

class QemuVirtioMMIONet : public QemuVirtioMMIO {
private:
    std::string netdev_id;
public:
    QemuVirtioMMIONet(sc_core::sc_module_name nm, QemuInstance &inst)
            : QemuVirtioMMIO(nm, inst, "virtio-net-device")
            , netdev_id(std::string(name()) + "-id")
    {
        std::stringstream opts;
        opts << "type=tap,id=" << netdev_id.c_str();
        m_inst.add_arg("-netdev");
        m_inst.add_arg(opts.str().c_str());
    }

    void before_end_of_elaboration() override
    {
        QemuVirtioMMIO::before_end_of_elaboration();
        QemuDevice::before_end_of_elaboration();
        m_dev.set_prop_str("netdev", netdev_id.c_str());
    }

    void end_of_elaboration() override
    {
        QemuVirtioMMIO::end_of_elaboration();
        qemu::Device virtio_net_dev(m_dev);
        virtio_net_dev.set_parent_bus(QemuVirtioMMIO::get_bus());
        QemuDevice::end_of_elaboration();
    }
};
