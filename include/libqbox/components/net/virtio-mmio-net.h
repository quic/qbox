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
#include "libqbox/qemu-instance.h"
#include <greensocs/gsutils/module_factory_registery.h>

class QemuVirtioMMIONet : public QemuVirtioMMIO
{
private:
    std::string netdev_id;
    cci::cci_param<std::string> netdev_str;

public:
    QemuVirtioMMIONet(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : QemuVirtioMMIONet(name, *(dynamic_cast<QemuInstance*>(o)))
        {
        }
    QemuVirtioMMIONet(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuVirtioMMIO(nm, inst, "virtio-net-device")
        , netdev_id(std::string(name()) + "-id")
        , netdev_str("netdev_str", "user,hostfwd=tcp::2222-:22",
                     "netdev string for QEMU (do not specify ID)") {
        std::stringstream opts;
        opts << netdev_str.get_value();
        opts << ",id=" << netdev_id;

        m_inst.add_arg("-netdev");
        m_inst.add_arg(opts.str().c_str());
    }

    void before_end_of_elaboration() override {
        QemuVirtioMMIO::before_end_of_elaboration();

        m_dev.set_prop_str("netdev", netdev_id.c_str());
    }
};

GSC_MODULE_REGISTER(QemuVirtioMMIONet, sc_core::sc_object*);
