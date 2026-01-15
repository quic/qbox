/*
 * This file is part of libqbox
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_DEVICE_H
#define _LIBQBOX_COMPONENTS_DEVICE_H

#include <systemc>

#include "qemu-instance.h"

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
    std::string m_id;

protected:
    QemuInstance& m_inst;
    qemu::Device m_dev;
    bool m_instanciated = false;
    bool m_realized = false;

public:
    void instantiate()
    {
        if (m_instanciated) {
            return;
        }

        m_dev = m_inst.get().object_new(m_qom_type.c_str(), m_id.c_str());
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

    void unrealize()
    {
        if (!m_realized) {
            return;
        }

        m_dev.set_prop_bool("realized", false);
        m_realized = false;
    }

    /**
     * @brief Construct a QEMU device
     *
     * @param[in] name SystemC module name
     * @param[in] inst QEMU instance the device will be created in
     * @param[in] qom_type Device QOM type name
     */
    QemuDevice(const sc_core::sc_module_name& name, QemuInstance& inst, const char* qom_type)
        : sc_module(name), m_qom_type(qom_type), m_inst(inst)
    {
        SCP_WARN(())("QOM Device creation {}", qom_type);
    }

    /**
     * @brief Construct a QEMU device
     *
     * @param[in] name SystemC module name
     * @param[in] inst QEMU instance the device will be created in
     * @param[in] qom_type Device QOM type name
     * @param[in] id Device QOM id
     */
    QemuDevice(const sc_core::sc_module_name& name, QemuInstance& inst, const char* qom_type, const char* id)
        : sc_module(name), m_qom_type(qom_type), m_inst(inst), m_id(id)
    {
        SCP_WARN(())("QOM Device creation {} with id: {}", qom_type, id);
    }

    virtual ~QemuDevice() {}

    virtual void before_end_of_elaboration() override { instantiate(); }

    virtual void end_of_elaboration() override { realize(); }

    void set_qom_type(std::string const& qom_type) { m_qom_type = qom_type; }

    const char* get_qom_type() const { return m_qom_type.c_str(); }

    const char* get_id() const { return m_id.c_str(); }

    void set_id(const std::string& nm) { m_id = nm; }

    qemu::Device get_qemu_dev() { return m_dev; }

    QemuInstance& get_qemu_inst() { return m_inst; }

    void set_sysbus_as_parent_bus(void) { m_dev.set_parent_bus(m_inst.get().sysbus_get_default()); }
};

#endif
