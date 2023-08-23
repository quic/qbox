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

class QemuVirtioMMIOBlk : public QemuVirtioMMIO
{
private:
    std::string blkdev_id;
    cci::cci_param<std::string> blkdev_str;

public:
    QemuVirtioMMIOBlk(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : QemuVirtioMMIOBlk(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    QemuVirtioMMIOBlk(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuVirtioMMIO(nm, inst, "virtio-blk-device")
        , blkdev_id(std::string(name()) + "-id")
        , blkdev_str("blkdev_str", "", "blkdev string for QEMU (do not specify ID)")
    {
        std::stringstream opts;
        opts << blkdev_str.get_value();
        opts << ",id=" << blkdev_id;

        m_inst.add_arg("-drive");
        m_inst.add_arg(opts.str().c_str());
    }

    void before_end_of_elaboration() override
    {
        QemuVirtioMMIO::before_end_of_elaboration();

        m_dev.set_prop_parse("drive", blkdev_id.c_str());
    }
};

GSC_MODULE_REGISTER(QemuVirtioMMIOBlk, sc_core::sc_object*);
