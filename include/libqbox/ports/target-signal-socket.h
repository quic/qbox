/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Luc Michel
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

#ifndef _LIBQBOX_PORTS_TARGET_SIGNAL_SOCKET_H
#define _LIBQBOX_PORTS_TARGET_SIGNAL_SOCKET_H

#include <functional>

#include <libqemu-cxx/libqemu-cxx.h>

#include <greensocs/gsutils/ports/target-signal-socket.h>

/**
 * @class QemuTargetSignalSocket
 *
 * @brief A QEMU input GPIO exposed as a TargetSignalSocket<bool>
 *
 * @details This class exposes an input GPIO of a QEMU device as a
 * TargetSignalSocket<bool>. It can be connected to an sc_core::sc_port<bool>
 * or a TargetInitiatorSocket<bool>. Modifications to this socket will be
 * reported to the wrapped GPIO.
 */
class QemuTargetSignalSocket : public TargetSignalSocket<bool> {
protected:
    qemu::Gpio m_gpio_in;

    void value_changed_cb(const bool &val)
    {
        m_gpio_in.set(val);
    }

    void init_with_gpio(qemu::Gpio gpio)
    {
        using namespace std::placeholders;

        m_gpio_in = gpio;

        auto cb = std::bind(&QemuTargetSignalSocket::value_changed_cb, this, _1);
        register_value_changed_cb(cb);
    }

public:
    QemuTargetSignalSocket(const char *name)
        : TargetSignalSocket(name)
    {}

    /**
     * @brief Initialize this socket with a device and a GPIO index
     *
     * @details This method initializes the socket using the given QEMU device
     * and the corresponding GPIO index in this device. See the QEMU API and
     * the device you want to wrap to know what index to use here.
     *
     * @param[in] dev The QEMU device
     * @param[in] gpio_idx The GPIO index within the device
     */
    void init(qemu::Device dev, int gpio_idx)
    {
        qemu::Gpio gpio(dev.get_gpio_in(gpio_idx));
        init_with_gpio(gpio);
    }

    /**
     * @brief Initialize this socket with a device, a GPIO namespace, and a GPIO index
     *
     * @details This method initializes the socket using the given QEMU device
     * and the corresponding GPIO (namespace, index) pair in this device. See
     * the QEMU API and the device you want to wrap to know what
     * namespace/index to use here.
     *
     * @param[in] dev The QEMU device
     * @param[in] gpio_name The GPIO namespace within the device
     * @param[in] gpio_idx The GPIO index within the device
     */
    void init_named(qemu::Device dev, const char *gpio_name, int gpio_idx)
    {
        qemu::Gpio gpio(dev.get_gpio_in_named(gpio_name, gpio_idx));
        init_with_gpio(gpio);
    }

    /**
     * @brief Returns the GPIO wrapped by this socket
     *
     * @return the GPIO wrapped by this socket
     */
    qemu::Gpio get_gpio()
    {
        return m_gpio_in;
    }

    /**
     * @brief Force a notification on the default event
     */
    void notify()
    {
        m_proxy.notify();
    }
};

#endif
