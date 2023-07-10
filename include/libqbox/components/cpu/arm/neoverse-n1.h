/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 GreenSocs
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

#include <greensocs/gsutils/tlm-extensions/exclusive-access.h>
#include <greensocs/gsutils/module_factory_registery.h>

#include "libqbox/components/cpu/cpu.h"
#include "libqbox/ports/initiator-signal-socket.h"
#include "libqbox/ports/target-signal-socket.h"
#include "libqbox/qemu-instance.h"

class QemuCpuArmNeoverseN1 : public QemuCpu
{
public:
    static constexpr qemu::Target ARCH = qemu::Target::AARCH64;

protected:
    int get_psci_conduit_val() const {
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

    void add_exclusive_ext(TlmPayload& pl) {
        ExclusiveAccessTlmExtension* ext = new ExclusiveAccessTlmExtension;
        ext->add_hop(m_cpu.get_index());
        pl.set_extension(ext);
    }

    static uint64_t extract_data_from_payload(const TlmPayload& pl) {
        uint8_t* ptr = pl.get_data_ptr() + pl.get_data_length() - 1;
        uint64_t ret = 0;

        /* QEMU never accesses more than 64 bits at the same time */
        assert(pl.get_data_length() <= 8);

        while (ptr >= pl.get_data_ptr()) {
            ret = (ret << 8) | *(ptr--);
        }

        return ret;
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

    QemuCpuArmNeoverseN1(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : QemuCpuArmNeoverseN1(name, *(dynamic_cast<QemuInstance*>(o)))
        {
        }
    QemuCpuArmNeoverseN1(sc_core::sc_module_name name, QemuInstance& inst)
        : QemuCpu(name, inst, "neoverse-n1-arm")
        , p_mp_affinity("mp_affinity", 0, "Multi-processor affinity value")
        , p_has_el2("has_el2", true, "ARM virtualization extensions")
        , p_has_el3("has_el3", true, "ARM secure-mode extensions")
        , p_start_powered_off("start_powered_off", false,
                              "Start and reset the CPU "
                              "in powered-off state")
        , p_psci_conduit("psci_conduit", "disabled",
                         "Set the QEMU PSCI conduit: "
                         "disabled->no conduit, "
                         "hvc->through hvc call, "
                         "smc->through smc call")
        , p_rvbar("rvbar", 0ull, "Reset vector base address register value")

        , irq_in("irq_in")
        , fiq_in("fiq_in")
        , virq_in("virq_in")
        , vfiq_in("vfiq_in")
        , irq_timer_phys_out("irq_timer_phys_out")
        , irq_timer_virt_out("irq_timer_virt_out")
        , irq_timer_hyp_out("irq_timer_hyp_out")
        , irq_timer_sec_out("irq_timer_sec_out") {
        m_external_ev |= irq_in->default_event();
        m_external_ev |= fiq_in->default_event();
        m_external_ev |= virq_in->default_event();
        m_external_ev |= vfiq_in->default_event();
    }

    void before_end_of_elaboration() override {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuAarch64 cpu(m_cpu);
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

    void end_of_elaboration() override {
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

    void initiator_customize_tlm_payload(TlmPayload& payload) override {
        uint64_t addr;
        qemu::CpuAarch64 arm_cpu(m_cpu);

        QemuCpu::initiator_customize_tlm_payload(payload);

        addr = payload.get_address();

        if (!arm_cpu.is_in_exclusive_context()) {
            return;
        }

        if (addr != arm_cpu.get_exclusive_addr()) {
            return;
        }

        /*
         * We are in the load/store pair of the cmpxchg of the exclusive store
         * implementation. Add the exclusive access extension to lock the
         * memory and check for exclusive store success in
         * initiator_tidy_tlm_payload.
         */
        add_exclusive_ext(payload);
    }

    void initiator_tidy_tlm_payload(TlmPayload& payload) override {
        using namespace tlm;

        ExclusiveAccessTlmExtension* ext;
        qemu::CpuAarch64 arm_cpu(m_cpu);

        QemuCpu::initiator_tidy_tlm_payload(payload);

        payload.get_extension(ext);
        bool exit_tb = false;

        if (ext == nullptr) {
            return;
        }

        if (payload.get_command() == TLM_WRITE_COMMAND) {
            auto sta = ext->get_exclusive_store_status();

            /* We just executed an exclusive store. Check its status */
            if (sta == ExclusiveAccessTlmExtension::EXCLUSIVE_STORE_FAILURE) {
                /*
                 * To actually make the exclusive store fails, we need to trick
                 * QEMU into thinking that the value at the store address has
                 * changed compared to when it did the corresponding ldrex (due
                 * to the way exclusive loads/stores are implemented in QEMU).
                 * For this, we simply smash the exclusive_val field of the ARM
                 * CPU state in case it currently matches with the value in
                 * memory.
                 */
                uint64_t exclusive_val = arm_cpu.get_exclusive_val();
                uint64_t mem_val = extract_data_from_payload(payload);
                uint64_t mask = (payload.get_data_length() == 8)
                                    ? -1
                                    : (1 << (8 * payload.get_data_length())) - 1;

                if ((exclusive_val & mask) == mem_val) {
                    arm_cpu.set_exclusive_val(~exclusive_val);

                    /*
                     * Exit the execution loop to force QEMU to re-do the
                     * store. This is necessary because we modify exclusive_val
                     * in the CPU env. This field is also mapped on a TCG
                     * global. Even though the qemu_st_ixx TCG opcs are marked
                     * TCG_OPF_CALL_CLOBBER, TCG does not reload the global
                     * after the store as I thought it would do. To force this,
                     * we exit the TB here so that the new exclusive_val value
                     * will be taken into account on the next execution.
                     */
                    exit_tb = true;
                }

                payload.set_response_status(TLM_OK_RESPONSE);
            }
        }

        payload.clear_extension(ext);
        delete ext;

        if (exit_tb) {
            /*
             * FIXME: exiting the CPU loop from here is a bit violent. The
             * caller won't have a chance to destruct its stack objects. The
             * object model should be reworked to allow exiting the loop
             * cleanly.
             */
            m_cpu.exit_loop_from_io();
        }
    }
};

GSC_MODULE_REGISTER(QemuCpuArmNeoverseN1, sc_core::sc_object*);