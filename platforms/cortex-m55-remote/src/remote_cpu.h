/*
 * Quic Module Cortex-M55
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#ifndef __REMOTE_CORTEX_M55__
#define __REMOTE_CORTEX_M55__

#include <systemc>
#include <tlm>
#include <cci_configuration>
#include <greensocs/libgsutils.h>
#include <limits>
#include <libqbox/components/cpu/arm/cortex-m55.h>
#include <libqbox/qemu-instance.h>
#include "greensocs/base-components/memory.h"
#include "greensocs/base-components/router.h"
#include "greensocs/base-components/remote.h"
#include "greensocs/base-components/pass.h"
#include <greensocs/gsutils/module_factory_registery.h>


class RemoteCPU : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    RemoteCPU(const sc_core::sc_module_name& n, sc_core::sc_object* obj)
        : RemoteCPU(n, *(dynamic_cast<QemuInstance*>(obj)))
    {
    }
    RemoteCPU(const sc_core::sc_module_name& n, QemuInstance& qemu_inst)
        : sc_core::sc_module(n)
        , m_broker(cci::cci_get_broker())
        , m_gdb_port("gdb_port", 0, "GDB port")
        , m_qemu_inst(qemu_inst)
        , m_router("router")
        , m_cpu("cpu", m_qemu_inst)
    {
        unsigned int m_irq_num = m_broker.get_param_handle(std::string(this->name()) + ".cpu.nvic.num_irq")
                                     .get_cci_value()
                                     .get_uint();

        if (!m_gdb_port.is_default_value()) m_cpu.p_gdb_port = m_gdb_port;

        SCP_INFO(()) << "number of irqs  = " << m_irq_num;

        m_router.initiator_socket.bind(m_cpu.m_nvic.socket);
        m_cpu.socket.bind(m_router.target_socket);
    }

private:
    cci::cci_broker_handle m_broker;
    cci::cci_param<int> m_gdb_port;
    QemuInstance& m_qemu_inst;
    gs::Router<> m_router;
    CpuArmCortexM55 m_cpu;
};
GSC_MODULE_REGISTER(RemoteCPU, sc_core::sc_object*);
#endif
