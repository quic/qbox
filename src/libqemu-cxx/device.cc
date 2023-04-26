/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  GreenSocs
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
#include "internals.h"

namespace qemu {

void Device::connect_gpio_out(int idx, Gpio gpio) {
    QemuDevice* qemu_dev = reinterpret_cast<QemuDevice*>(m_obj);
    QemuGpio* qemu_gpio = reinterpret_cast<QemuGpio*>(gpio.get_qemu_obj());

    m_int->exports().qdev_connect_gpio_out(qemu_dev, idx, qemu_gpio);
}

void Device::connect_gpio_out_named(const char* n, int idx, Gpio gpio) {
    QemuDevice* qemu_dev = reinterpret_cast<QemuDevice*>(m_obj);
    QemuGpio* qemu_gpio = reinterpret_cast<QemuGpio*>(gpio.get_qemu_obj());

    m_int->exports().qdev_connect_gpio_out_named(qemu_dev, n, idx, qemu_gpio);
}

Gpio Device::get_gpio_in(int idx) {
    QemuDevice* qemu_dev = reinterpret_cast<QemuDevice*>(m_obj);
    QemuGpio* qemu_gpio = nullptr;

    qemu_gpio = m_int->exports().qdev_get_gpio_in(qemu_dev, idx);
    Object obj(reinterpret_cast<QemuObject*>(qemu_gpio), m_int);

    return Gpio(obj);
}

Gpio Device::get_gpio_in_named(const char* name, int idx) {
    QemuDevice* qemu_dev = reinterpret_cast<QemuDevice*>(m_obj);
    QemuGpio* qemu_gpio = nullptr;

    qemu_gpio = m_int->exports().qdev_get_gpio_in_named(qemu_dev, name, idx);
    Object obj(reinterpret_cast<QemuObject*>(qemu_gpio), m_int);

    return Gpio(obj);
}

Bus Device::get_child_bus(const char* name) {
    QemuDevice* qemu_dev = reinterpret_cast<QemuDevice*>(m_obj);
    QemuBus* qemu_bus = nullptr;

    qemu_bus = m_int->exports().qdev_get_child_bus(qemu_dev, name);
    Object obj(reinterpret_cast<QemuObject*>(qemu_bus), m_int);

    return Bus(obj);
}

void Device::set_parent_bus(Bus bus) {
    QemuDevice* qemu_dev = reinterpret_cast<QemuDevice*>(m_obj);
    QemuBus* qemu_bus = reinterpret_cast<QemuBus*>(bus.get_qemu_obj());

    m_int->exports().qdev_set_parent_bus(qemu_dev, qemu_bus);
}

void Device::set_prop_chardev(const char* name, Chardev chr) {
    QemuDevice* qemu_dev = reinterpret_cast<QemuDevice*>(m_obj);
    QemuChardev* char_dev = reinterpret_cast<QemuChardev*>(chr.get_qemu_obj());

    m_int->exports().qdev_prop_set_chr(qemu_dev, name, char_dev);
}

} // namespace qemu