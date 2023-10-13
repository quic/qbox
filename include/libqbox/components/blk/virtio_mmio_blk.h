/*
 *  This file is part of libqbox
 *  Copyright (c) 2022 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

class virtio_mmio_blk : public QemuVirtioMMIO
{
private:
    std::string blkdev_id;
    cci::cci_param<std::string> blkdev_str;

public:
    virtio_mmio_blk(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : virtio_mmio_blk(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    virtio_mmio_blk(sc_core::sc_module_name nm, QemuInstance& inst)
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

extern "C" void module_register();
