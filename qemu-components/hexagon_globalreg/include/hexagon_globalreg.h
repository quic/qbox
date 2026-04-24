/*
 * This file is part of libqbox
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_HEXAGON_GLOBALREG_INITIATOR_H
#define _LIBQBOX_COMPONENTS_HEXAGON_GLOBALREG_INITIATOR_H

#include <device.h>
#include <qemu-instance.h>
#include <module_factory_registery.h>
#include <libqemu-cxx/target/hexagon.h>
#include <qemu_hexagon_qtimer.h>
#include <hexagon-l2vic.h>

class hexagon_globalreg : public QemuDevice
{
private:
    SCP_LOGGER();
    qemu_hexagon_qtimer* m_qtimer;
    hexagon_l2vic* m_l2vic;

public:
    cci::cci_param<uint64_t> p_config_table_addr;
    cci::cci_param<uint32_t> p_hexagon_start_addr;
    cci::cci_param<std::string> p_dsp_arch;
    cci::cci_param<bool> p_isdben_trusted;
    cci::cci_param<bool> p_isdben_secure;

    hexagon_globalreg(const sc_core::sc_module_name& name, sc_core::sc_object* o,
                      sc_core::sc_object* qtimer_obj = nullptr, sc_core::sc_object* l2vic_obj = nullptr)
        : hexagon_globalreg(name, *(dynamic_cast<QemuInstance*>(o)), dynamic_cast<qemu_hexagon_qtimer*>(qtimer_obj),
                            dynamic_cast<hexagon_l2vic*>(l2vic_obj))
    {
    }
    hexagon_globalreg(const sc_core::sc_module_name& n, QemuInstance& inst, qemu_hexagon_qtimer* qtimer = nullptr,
                      hexagon_l2vic* l2vic = nullptr)
        : QemuDevice(n, inst, "hexagon-globalreg")
        , m_qtimer(qtimer)
        , m_l2vic(l2vic)
        , p_config_table_addr("config_table_addr", 0xffffffffULL, "config table address")
        , p_hexagon_start_addr("hexagon_start_addr", 0xffffffffULL, "execution start address")
        , p_dsp_arch("dsp_arch", "v68", "DSP arch")
        , p_isdben_trusted("isdben_trusted", true, "isdben trusted")
        , p_isdben_secure("isdben_secure", true, "isdben secure")
    {
        SCP_TRACE(())("Init");
    }

    void set_qtimer(qemu_hexagon_qtimer* qtimer) { m_qtimer = qtimer; }
    void set_l2vic(hexagon_l2vic* l2vic) { m_l2vic = l2vic; }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();
        qemu::Device hex_greg_devs = get_qemu_dev();
        const std::string dsp_arch = p_dsp_arch.get_value();
        qemu::CpuHexagon::Rev_t dsp_rev = qemu::CpuHexagon::parse_dsp_arch(dsp_arch);
        if (dsp_rev == qemu::CpuHexagon::unknown_rev) {
            SCP_FATAL(())("Unrecognized Architecture Revision: " + dsp_arch);
        }
        hex_greg_devs.set_prop_int("config-table-addr", p_config_table_addr);
        hex_greg_devs.set_prop_int("boot-evb", p_hexagon_start_addr);
        hex_greg_devs.set_prop_int("dsp-rev", dsp_rev);
        hex_greg_devs.set_prop_bool("isdben-trusted", p_isdben_trusted);
        hex_greg_devs.set_prop_bool("isdben-secure", p_isdben_secure);

        if (m_qtimer) {
            m_qtimer->before_end_of_elaboration();
            hex_greg_devs.set_prop_link("qtimer", m_qtimer->get_qemu_dev());
        }
        if (m_l2vic) {
            m_l2vic->before_end_of_elaboration();
            hex_greg_devs.set_prop_link("l2vic", m_l2vic->get_qemu_dev());
        }
    }
};

extern "C" void module_register();

#endif //_LIBQBOX_COMPONENTS_HEXAGON_GLOBALREG_INITIATOR_H
