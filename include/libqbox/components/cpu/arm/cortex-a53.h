/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps and Luc Michel
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

#pragma once

#include <string>

#include <cci_configuration>

#include <libqemu-cxx/target/aarch64.h>

#include "libqbox/components/cpu/cpu.h"
#include "libqbox/ports/initiator-signal-socket.h"
#include "libqbox/ports/target-signal-socket.h"

class QemuCpuArmCortexA53 : public QemuCpu {
public:
    static constexpr qemu::Target ARCH = qemu::Target::AARCH64;

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
    cci::cci_param<unsigned int> p_mp_affinity;
    cci::cci_param<bool> p_has_el2;
    cci::cci_param<bool> p_has_el3;
    cci::cci_param<bool> p_start_powered_off;
    cci::cci_param<std::string> p_psci_conduit;
    cci::cci_param<uint64_t> p_rvbar;

    QemuTargetSignalSocket irq_in;
    QemuTargetSignalSocket fiq_in;
    QemuTargetSignalSocket virq_in;
    QemuTargetSignalSocket vfiq_in;

    QemuInitiatorSignalSocket irq_timer_phys_out;
    QemuInitiatorSignalSocket irq_timer_virt_out;
    QemuInitiatorSignalSocket irq_timer_hyp_out;
    QemuInitiatorSignalSocket irq_timer_sec_out;

	QemuCpuArmCortexA53(sc_core::sc_module_name name, QemuInstance &inst)
		: QemuCpu(name, inst, "cortex-a53-arm")
        , p_mp_affinity("mp-affinity", 0, "Multi-processor affinity value")
        , p_has_el2("has_el2", true, "ARM virtualization extensions")
        , p_has_el3("has_el3", true, "ARM secure-mode extensions")
        , p_start_powered_off("start-powered-off", false, "Start and reset the CPU "
                                                          "in powered-off state")
        , p_psci_conduit("psci-conduit", "disabled", "Set the QEMU PSCI conduit: "
                                                     "disabled->no conduit, "
                                                     "hvc->through hvc call, "
                                                     "smc->through smc call")
        , p_rvbar("rvbar", 0ull, "Reset vector base address register value")

        , irq_in("irq-in")
        , fiq_in("fiq-in")
        , virq_in("virq-in")
        , vfiq_in("vfiq-in")
        , irq_timer_phys_out("irq-timer-phys-out")
        , irq_timer_virt_out("irq-timer-virt-out")
        , irq_timer_hyp_out("irq-timer-hyp-out")
        , irq_timer_sec_out("irq-timer-sec-out")
	{
        m_external_ev |= irq_in->default_event();
        m_external_ev |= fiq_in->default_event();
        m_external_ev |= virq_in->default_event();
        m_external_ev |= vfiq_in->default_event();
    }

    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuAarch64 cpu = qemu::CpuAarch64(get_qemu_dev());
        cpu.set_aarch64_mode(true);

        if (!p_mp_affinity.is_default_value()) {
            cpu.set_prop_int("mp-affinity", p_mp_affinity);
        }
        cpu.set_prop_bool("has_el2", p_has_el2);
        cpu.set_prop_bool("has_el3", p_has_el3);

        cpu.set_prop_bool("start-powered-off", p_start_powered_off);
        cpu.set_prop_int("psci-conduit", get_psci_conduit_val());

        cpu.set_prop_int("rvbar", p_rvbar);
    }

    void end_of_elaboration()
    {
        QemuCpu::end_of_elaboration();

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
