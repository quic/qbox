/*
 *  This file is part of libqbox
 *  Copyright (c) 2022 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <vector>

#include <cci_configuration>

#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>

#include <cciutils.h>
#include <scp/report.h>

class QemuVirtioMMIO : public QemuDevice
{
    SCP_LOGGER();

public:
    QemuTargetSocket<> socket;
    QemuInitiatorSignalSocket irq_out;
    QemuDevice virtio_mmio_device;

    /*
     * qemu qbus hierachy created behind this sc_module:
     * virtio_mmio_device
     *   + qemu-type: "virtio-mmio" (this is a sysbus device)
     *   + qbus-child(&qon-child) "virtio-mmio-bus":
     *       + qdev-child (this QemuVirtioMMIO object):
     *       + qemu-type: "virtio-net-device"
     */
    QemuVirtioMMIO(sc_core::sc_module_name nm, QemuInstance& inst, const char* device_type)
        : QemuDevice(nm, inst, device_type)
        , socket("mem", inst)
        , irq_out("irq_out")
        , virtio_mmio_device("virtio_mmio", inst, "virtio-mmio")
    {
        SCP_TRACE(())("Constructor");
    }

    void before_end_of_elaboration() override
    {
        virtio_mmio_device.instantiate();
        QemuDevice::before_end_of_elaboration();
        for (auto n : gs::sc_cci_children((std::string(name()) + ".device_properties").c_str())) {
            cci::cci_value v = gs::cci_get(cci::cci_get_broker(), std::string(name()) + ".device_properties." + n);
            /* NB we could use _parse here,but JSON strings dont always match QEMU command line strings! */
            if (v.is_bool()) {
                SCP_DEBUG(())("Setting bool {} to {}", n, v.to_json());
                m_dev.set_prop_bool(n.c_str(), v.get_bool());
                continue;
            }
            if (v.is_number()) {
                SCP_DEBUG(())("Setting number {} to {}", n, v.to_json());
                m_dev.set_prop_int(n.c_str(), v.get_uint64());
                continue;
            }
            if (v.is_string()) {
                SCP_DEBUG(())("Setting string {} to {}", n, v.to_json());
                m_dev.set_prop_str(n.c_str(), v.get_string().c_str());
                continue;
            }
            SCP_WARN(())("Ignoring property {}, unknown type. {}", n, v.to_json());
        }

        virtio_mmio_device.get_qemu_dev().set_prop_bool("force-legacy", true);

        // force MMIO devices to use transport address (which will be 0, causing automatic ID)
        virtio_mmio_device.get_qemu_dev().set_prop_bool("format_transport_address", false);
    }

    void end_of_elaboration() override
    {
        /*
         * we realize virtio_mmio_device first because
         * it creates the "virtio-mmio-bus" we need below
         */
        virtio_mmio_device.set_sysbus_as_parent_bus();
        virtio_mmio_device.realize();

        /*
         * Expose the sysbus device mmio & irq
         */
        qemu::SysBusDevice sbd(virtio_mmio_device.get_qemu_dev());
        socket.init(sbd, 0);
        irq_out.init_sbd(sbd, 0);

        /*
         * Realize the true virtio net object
         */
        qemu::Device virtio_device(m_dev);
        virtio_device.set_parent_bus(QemuVirtioMMIO::get_bus());
        QemuDevice::end_of_elaboration();
    }

private:
    qemu::Bus get_bus()
    {
        qemu::Device virtio_mmio_dev(virtio_mmio_device.get_qemu_dev());
        return virtio_mmio_dev.get_prop_link("virtio-mmio-bus");
    }
};
