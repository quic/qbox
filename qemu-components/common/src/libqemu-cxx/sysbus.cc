/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libqemu/libqemu.h>

#include <libqemu-cxx/libqemu-cxx.h>
#include <internals.h>

namespace qemu {

MemoryRegion SysBusDevice::mmio_get_region(int id)
{
    QemuSysBusDevice* qemu_sbd;
    QemuMemoryRegion* qemu_mr;

    qemu_sbd = reinterpret_cast<QemuSysBusDevice*>(m_obj);
    qemu_mr = m_int->exports().sysbus_mmio_get_region(qemu_sbd, id);

    if (qemu_mr == nullptr) {
        throw LibQemuException("Error while getting MMIO region from SysBusDevice");
    }

    Object o(reinterpret_cast<QemuObject*>(qemu_mr), m_int);

    return MemoryRegion(o);
}

void SysBusDevice::connect_gpio_out(int idx, Gpio gpio)
{
    QemuSysBusDevice* qemu_sbd;
    QemuGpio* qemu_gpio;

    qemu_sbd = reinterpret_cast<QemuSysBusDevice*>(m_obj);
    qemu_gpio = reinterpret_cast<QemuGpio*>(gpio.get_qemu_obj());

    m_int->exports().sysbus_connect_gpio_out(qemu_sbd, idx, qemu_gpio);
}

}; // namespace qemu
