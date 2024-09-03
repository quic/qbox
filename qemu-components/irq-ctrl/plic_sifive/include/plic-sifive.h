/*
 * This file is part of libqbox
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_IRQ_CTRL_PLIC_SIFIVE_H
#define _LIBQBOX_COMPONENTS_IRQ_CTRL_PLIC_SIFIVE_H

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <string>
#include <cassert>

#include <cci_configuration>

#include <libqemu-cxx/target/riscv.h>
#include <module_factory_registery.h>
#include <device.h>

#include <ports/target.h>
#include <ports/qemu-target-signal-socket.h>

class plic_sifive : public QemuDevice
{
public:
    cci::cci_param<unsigned int> p_num_sources;
    cci::cci_param<unsigned int> p_num_priorities;
    cci::cci_param<uint64_t> p_priority_base;
    cci::cci_param<uint64_t> p_pending_base;
    cci::cci_param<uint64_t> p_enable_base;
    cci::cci_param<uint64_t> p_enable_stride;
    cci::cci_param<uint64_t> p_context_base;
    cci::cci_param<uint64_t> p_context_stride;
    cci::cci_param<uint64_t> p_aperture_size;
    cci::cci_param<std::string> p_hart_config;

    QemuTargetSocket<> socket;
    sc_core::sc_vector<QemuTargetSignalSocket> irq_in;

    plic_sifive(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : plic_sifive(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    plic_sifive(sc_core::sc_module_name nm, QemuInstance& inst)
        : QemuDevice(nm, inst, "riscv.sifive.plic")
        , p_num_sources("num_sources", 0, "Number of input IRQ lines")
        , p_num_priorities("num_priorities", 0, "Number of priorities")
        , p_priority_base("priority_base", 0, "Base address of the priority registers")
        , p_pending_base("pending_base", 0, "Base address of the pending registers")
        , p_enable_base("enable_base", 0, "Base address of the enable registers")
        , p_enable_stride("enable_stride", 0, "Size of the enable regiters")
        , p_context_base("context_base", 0, "Base address the context registers")
        , p_context_stride("context_stride", 0, "Size of the context registers")
        , p_aperture_size("aperture_size", 0, "Size of the whole PLIC address space")
        , p_hart_config("hart_config", "",
                        "HART configurations (can be U, S, H or M or "
                        "a combination of those, each HART config is "
                        "separarted by a comma) (example: \"MS,MS\" -> "
                        "two HARTs with M and S mode)")
        , socket("mem", inst)
        , irq_in("irq_in", p_num_sources, [](const char* n, int i) { return new QemuTargetSignalSocket(n); })
    {
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_str("hart-config", p_hart_config.get_value().c_str());
        m_dev.set_prop_int("num-sources", p_num_sources);
        m_dev.set_prop_int("aperture-size", p_aperture_size);
        m_dev.set_prop_int("num-priorities", p_num_priorities);
        m_dev.set_prop_int("priority-base", p_priority_base);
        m_dev.set_prop_int("pending-base", p_pending_base);
        m_dev.set_prop_int("enable-base", p_enable_base);
        m_dev.set_prop_int("enable-stride", p_enable_stride);
        m_dev.set_prop_int("context-base", p_context_base);
        m_dev.set_prop_int("context-stride", p_context_stride);
    }

    void end_of_elaboration() override
    {
        int i;

        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_dev());
        socket.init(qemu::SysBusDevice(m_dev), 0);

        for (i = 0; i < p_num_sources; i++) {
            irq_in[i].init(m_dev, i);
        }
    }
};

extern "C" void module_register();

#endif
