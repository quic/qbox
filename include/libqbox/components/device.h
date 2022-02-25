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

#ifndef _LIBQBOX_COMPONENTS_DEVICE_H
#define _LIBQBOX_COMPONENTS_DEVICE_H

#include <systemc>

#include "libqbox/qemu-instance.h"

/**
 * @class QemuDevice
 *
 * @brief QEMU device abstraction as a SystemC module
 *
 * @details This class abstract a QEMU device as a SystemC module. It is
 * constructed using the QEMU instance it will lie in, and the QOM type name
 * corresponding to the device. This class is meant to be inherited from by
 * children classes that implement a given device.
 *
 * The elaboration flow is as follows:
 *   * At construct time, nothing happen on the QEMU side.
 *   * When before_end_of_elaboration is called, the QEMU object correponding
 *     to this component is created. Children classes should always call the
 *     parent method when overriding it. Usually, they start by calling it and
 *     then set the QEMU properties on the device.
 *   * When end_of_elaboration is called, the device is realized. No more
 *     property can be set (unless particular cases such as some link
 *     properties) and the device can now be connected to busses and GPIO.
 */
class QemuDevice : public sc_core::sc_module, public QemuDeviceBaseIF
{
private:
    std::string m_qom_type;

protected:
    QemuInstance &m_inst;
    qemu::Device m_dev;
    bool m_instanciated = false;
    bool m_realized = false;

    void instantiate()
    {
        if (m_instanciated) {
            return;
        }

        m_dev = m_inst.get().object_new(m_qom_type.c_str());
        m_instanciated = true;
    }

    void realize()
    {
        if (m_realized) {
            return;
        }

        m_dev.set_prop_bool("realized", true);
        m_realized = true;
    }

public:
    /**
     * @brief Construct a QEMU device
     *
     * @param[in] name SystemC module name
     * @param[in] inst QEMU instance the device will be created in
     * @param[in] qom_type Device QOM type name
     */
    QemuDevice(const sc_core::sc_module_name &name, QemuInstance &inst,
               const char* qom_type)
        : sc_module(name)
        , m_qom_type(qom_type)
        , m_inst(inst)
    {}

    virtual ~QemuDevice() {}

    virtual void before_end_of_elaboration() override
    {
        instantiate();
    }

    virtual void end_of_elaboration() override
    {
        realize();
    }

    const char * get_qom_type() const
    {
        return m_qom_type.c_str();
    }

    qemu::Device get_qemu_dev()
    {
        return m_dev;
    }

    QemuInstance& get_qemu_inst()
    {
        return m_inst;
    }
};

#endif
