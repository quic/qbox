/*
 * Quic Module Cortex-M55
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
    gs::router<> m_router;
    CpuArmCortexM55 m_cpu;
};
GSC_MODULE_REGISTER(RemoteCPU, sc_core::sc_object*);
#endif
