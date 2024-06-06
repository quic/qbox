/*
 * This file is part of libqemu-cxx
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2015-2019
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libqemu/libqemu.h>

#include <libqemu-cxx/libqemu-cxx.h>
#include <internals.h>

namespace qemu {

void Gpio::set(bool lvl)
{
    QemuGpio* gpio = reinterpret_cast<QemuGpio*>(m_obj);

    m_int->exports().gpio_set(gpio, lvl);
}

}; // namespace qemu
