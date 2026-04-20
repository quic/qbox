/*
 * Self-contained arm_gicv2m QBox module.
 *
 * Wraps QEMU's "arm-gicv2m" device as an independently loadable DLL module.
 * When a GIC reference (v2 or v3) is passed as a constructor argument, SPI
 * outputs are automatically bound to the GIC's SPI inputs.
 *
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_IRQ_CTRL_ARM_GICV2M_H
#define _LIBQBOX_COMPONENTS_IRQ_CTRL_ARM_GICV2M_H

#include <string>

#include <systemc>
#include <cci_configuration>

#include <scp/report.h>

#include <module_factory_registery.h>

#include <cciutils.h>
#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <ports/qemu-target-signal-socket.h>

class arm_gicv2m : public QemuDevice
{
protected:
    cci::cci_param<unsigned int> p_base_spi;
    cci::cci_param<unsigned int> p_num_spis;

private:
    sc_core::sc_object* m_gic;

public:
    sc_core::sc_vector<QemuInitiatorSignalSocket> spi_out;

    QemuTargetSocket<> iface;

    arm_gicv2m(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* gic)
        : arm_gicv2m(name, *(dynamic_cast<QemuInstance*>(o)), gic)
    {
    }

    arm_gicv2m(const sc_core::sc_module_name& name, QemuInstance& inst, sc_core::sc_object* gic = nullptr)
        : QemuDevice(name, inst, "arm-gicv2m")
        , p_base_spi("base_spi", 0, "Start index of gic's spis")
        , p_num_spis("num_spis", 0, "Number of gic's spis")
        , m_gic(gic)
        , spi_out("spi_out", p_num_spis)
        , iface("iface", inst)
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_int("base-spi", p_base_spi);
        m_dev.set_prop_int("num-spi", p_num_spis);

        if (m_gic) {
            for (unsigned int i = 0; i < p_num_spis; i++) {
                std::string spi_name = std::string(m_gic->name()) + ".spi_in_" + std::to_string(p_base_spi + i);
                sc_core::sc_object* obj = gs::find_sc_obj(nullptr, spi_name);
                if (!obj) {
                    SCP_FATAL(SCMOD) << "Cannot find " << spi_name << " for auto-bind";
                }
                auto* target = dynamic_cast<QemuTargetSignalSocket*>(obj);
                if (!target) {
                    SCP_FATAL(SCMOD) << spi_name << " is not a QemuTargetSignalSocket";
                }
                spi_out[i].bind(*target);
            }
        }
    }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);

        iface.init(sbd, 0);

        for (unsigned int i = 0; i < p_num_spis; i++) {
            spi_out[i].init_sbd(sbd, i);
        }
    }
};

extern "C" void module_register();

#endif
