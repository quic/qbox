/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_PORTS_INITIATOR_SIGNAL_SOCKET_H
#define _LIBQBOX_PORTS_INITIATOR_SIGNAL_SOCKET_H

#include <functional>
#include <cassert>

#include <libqemu-cxx/libqemu-cxx.h>

#include <ports/target-signal-socket.h>
#include <ports/initiator-signal-socket.h>

#include <libgssync.h>

#include <ports/qemu-target-signal-socket.h>

/**
 * @class QemuInitiatorSignalSocket
 *
 * @brief A QEMU output GPIO exposed as a InitiatorSignalSocket<bool>
 *
 * @details This class exposes an output GPIO of a QEMU device as a
 * InitiatorSignalSocket<bool>. It can be connected to an sc_core::sc_port<bool>
 * or a TargetSignalSocket<bool>. Modifications to the interal QEMU GPIO will
 * be propagated through the socket.
 *
 * If this socket happens to be connected to a QemuTargetSignalSocket, the
 * propagation is done directly within QEMU and do not go through the SystemC
 * kernel. Note that this is only true if the GPIOs wrapped by both this socket
 * and the remote socket lie in the same QEMU instance.
 */
class QemuInitiatorSignalSocket : public InitiatorSignalSocket<bool>
{
protected:
    qemu::Gpio m_proxy;
    gs::runonsysc m_on_sysc;
    QemuTargetSignalSocket* m_qemu_remote = nullptr;

    void event_cb(bool val)
    {
        if (m_qemu_remote && (m_qemu_remote->get_gpio().same_inst_as(m_proxy))) {
            /*
             * We make sure to be in the same instance as the target socket.
             * Otherwise we would need to hold the target's QEMU instance
             * iothread lock. This would require us to unlock our own lock
             * first to ensure we won't deadlock because of a race condition
             * between two initiators trying to set a GPIO in one another.
             * Don't bother with that. Instead, use the SystemC kernel as a
             * synchronization point in this case.
             */
            m_qemu_remote->get_gpio().set(val);

            m_on_sysc.run_on_sysc([this] { m_qemu_remote->notify(); }, false);

            return;
        }

        if (!get_interface()) {
            return;
        }

        m_proxy.get_inst().unlock_iothread();

        m_on_sysc.run_on_sysc([this, val] { (*this)->write(val); });

        m_proxy.get_inst().lock_iothread();
    }

    void init_qemu_to_sysc_gpio_proxy(qemu::Device& dev)
    {
        using namespace std::placeholders;

        m_proxy = dev.get_inst().gpio_new();

        auto cb = std::bind(&QemuInitiatorSignalSocket::event_cb, this, _1);
        m_proxy.set_event_callback(cb);
    }

    void init_internal(qemu::Device& dev)
    {
        sc_core::sc_interface* iface;
        TargetSignalSocketProxy<bool>* socket_iface;
        QemuTargetSignalSocket* remote;

        init_qemu_to_sysc_gpio_proxy(dev);

        iface = get_interface();

        /* Check if we're bound to a TargetSignalSocket<bool> */
        socket_iface = dynamic_cast<TargetSignalSocketProxy<bool>*>(iface);

        if (socket_iface == nullptr) {
            return;
        }

        /* Check if we're bound to a QemuTargetSignalSocket */
        remote = dynamic_cast<QemuTargetSignalSocket*>(&socket_iface->get_parent());

        if (remote == nullptr) {
            return;
        }

        /*
         * We could connect the GPIOs from within QEMU directly, but we don't
         * know if the remote initialization is done yet. Instead, keep a
         * pointer on the remote to change its GPIO directly in event_cb()
         * instead of going through the SystemC kernel.
         */
        m_qemu_remote = remote;
    }

public:
    QemuInitiatorSignalSocket(const char* name)
        : InitiatorSignalSocket<bool>(name), m_on_sysc(sc_core::sc_gen_unique_name("run_on_sysc"))
    {
    }

    ~QemuInitiatorSignalSocket() { m_proxy.set_event_callback(nullptr); }

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
        init_internal(dev);
        dev.connect_gpio_out(gpio_idx, m_proxy);
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
    void init_named(qemu::Device dev, const char* gpio_name, int gpio_idx)
    {
        init_internal(dev);
        dev.connect_gpio_out_named(gpio_name, gpio_idx, m_proxy);
    }

    /**
     * @brief Initialize this socket with a QEMU SysBusDevice, and a GPIO index
     *
     * @details This method initializes the socket using the given QEMU
     * SysBusDevice (SBD) and the corresponding GPIO index) in this SBD. See
     * the QEMU API and the SBD you want to wrap to know what index to use
     * here. This is only for "sysbus_irq", if you want to wrap a normal gpio,
     * just use init or init_named.
     *
     * @param[in] sbd The QEMU SysBusDevice
     * @param[in] gpio_idx The GPIO index within the SBD
     */
    void init_sbd(qemu::SysBusDevice sbd, int gpio_idx)
    {
        init_internal(sbd);
        sbd.connect_gpio_out(gpio_idx, m_proxy);
    }
};

#endif
