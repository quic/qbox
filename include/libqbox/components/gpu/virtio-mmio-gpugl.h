/*
 *  This file is part of libqbox
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

class QemuVirtioMMIOGpuGl : public QemuVirtioMMIO
{
private:
public:
    QemuVirtioMMIOGpuGl(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuVirtioMMIO(nm, inst, "virtio-gpu-gl-device")
    {
    }

    void before_end_of_elaboration() override { QemuVirtioMMIO::before_end_of_elaboration(); }
};
