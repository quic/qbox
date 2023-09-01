/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libqemu/libqemu.h>

#include "libqemu-cxx/libqemu-cxx.h"
#include "internals.h"

namespace qemu {

void GpexHost::set_irq_num(int idx, int gic_irq)
{
    QemuSysBusDevice* qemu_sbd;

    qemu_sbd = reinterpret_cast<QemuSysBusDevice*>(m_obj);

    m_int->exports().gpex_set_irq_num(qemu_sbd, idx, gic_irq);
}

} // namespace qemu
