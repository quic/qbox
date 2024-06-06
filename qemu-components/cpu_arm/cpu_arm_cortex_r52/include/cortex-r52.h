/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>

#include <cci_configuration>

#include <module_factory_registery.h>

#include <libqemu-cxx/target/aarch64.h>

#include <arm.h>
#include <device.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <ports/qemu-target-signal-socket.h>
#include <ports/target.h>

class cpu_arm_cortexR52 : public QemuCpuArm
{
protected:
    int get_psci_conduit_val() const
    {
        if (p_psci_conduit.get_value() == "disabled") {
            return 0;
        } else if (p_psci_conduit.get_value() == "smc") {
            return 1;
        } else if (p_psci_conduit.get_value() == "hvc") {
            return 2;
        } else {
            /* TODO: report warning */
            return 0;
        }
    }

public:
    cci::cci_param<bool> p_start_powered_off;
    cci::cci_param<uint64_t> p_rvbar;
    cci::cci_param<bool> p_has_el2;
    cci::cci_param<std::string> p_psci_conduit;

    QemuTargetSignalSocket irq_in;
    QemuTargetSignalSocket fiq_in;
    QemuTargetSignalSocket virq_in;
    QemuTargetSignalSocket vfiq_in;

    QemuInitiatorSignalSocket irq_timer_phys_out;
    QemuInitiatorSignalSocket irq_timer_virt_out;
    QemuInitiatorSignalSocket irq_timer_hyp_out;
    QemuInitiatorSignalSocket irq_timer_sec_out;

    cpu_arm_cortexR52(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : cpu_arm_cortexR52(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    cpu_arm_cortexR52(sc_core::sc_module_name name, QemuInstance& inst)
        : QemuCpuArm(name, inst, "cortex-r52-arm")
        , p_has_el2("has_el2", true, "ARM virtualization extensions")
        , p_rvbar("rvbar", 0ull, "Reset vector base address register value")
        , p_start_powered_off("start_powered_off", false,
                              "Start and reset the CPU "
                              "in powered-off state")
        , p_psci_conduit("psci_conduit", "disabled",
                         "Set the QEMU PSCI conduit: "
                         "disabled->no conduit, "
                         "hvc->through hvc call, "
                         "smc->through smc call")
        , irq_in("irq_in")
        , fiq_in("fiq_in")
        , virq_in("virq_in")
        , vfiq_in("vfiq_in")
        , irq_timer_phys_out("irq_timer_phys_out")
        , irq_timer_virt_out("irq_timer_virt_out")
        , irq_timer_hyp_out("irq_timer_hyp_out")
        , irq_timer_sec_out("irq_timer_sec_out")
    {
    }

    void before_end_of_elaboration() override
    {
        QemuCpuArm::before_end_of_elaboration();

        qemu::CpuArm cpu(m_dev);

        cpu.set_prop_bool("start-powered-off", p_start_powered_off);
        cpu.set_prop_int("rvbar", p_rvbar);
        cpu.set_prop_int("psci-conduit", get_psci_conduit_val());
    }

    void end_of_elaboration() override
    {
        QemuCpuArm::end_of_elaboration();

        irq_in.init(m_dev, 0);
        fiq_in.init(m_dev, 1);
        virq_in.init(m_dev, 2);
        vfiq_in.init(m_dev, 3);

        irq_timer_phys_out.init(m_dev, 0);
        irq_timer_virt_out.init(m_dev, 1);
        irq_timer_hyp_out.init(m_dev, 2);
        irq_timer_sec_out.init(m_dev, 3);
    }
};

extern "C" void module_register();
