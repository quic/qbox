/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  Clement Deschamps and Luc Michel
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

#include <libqemu/libqemu.h>

#include "libqemu-cxx/libqemu-cxx.h"

namespace qemu {

MemoryRegion SysBusDevice::mmio_get_region(int id)
{
    QemuSysBusDevice *qemu_sbd;
    QemuMemoryRegion *qemu_mr;

    qemu_sbd = reinterpret_cast<QemuSysBusDevice *>(m_obj);
    qemu_mr = m_exports->sysbus_mmio_get_region(qemu_sbd, id);

    if (qemu_mr == nullptr) {
        throw LibQemuException("Error while getting MMIO region from SysBusDevice");
    }

    Object o(reinterpret_cast<QemuObject *>(qemu_mr), *m_exports);

    return MemoryRegion(o);
}

void SysBusDevice::connect_gpio_out(int idx, Gpio gpio)
{
    QemuSysBusDevice *qemu_sbd;
    QemuGpio *qemu_gpio;

    qemu_sbd = reinterpret_cast<QemuSysBusDevice *>(m_obj);
    qemu_gpio = reinterpret_cast<QemuGpio *>(gpio.get_qemu_obj());

    m_exports->sysbus_connect_gpio_out(qemu_sbd, idx, qemu_gpio);
}

};
