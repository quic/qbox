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

#include <systemc>
#include <tlm>
#include <cci_configuration>
#include <greensocs/libgsutils.h>
#include <limits>
#include <libqbox/components/cpu/arm/cortex-m55.h>
#include <greensocs/gsutils/module_factory_container.h>
#include <libqbox/qemu-instance.h>
#include "greensocs/base-components/memory.h"
#include "greensocs/base-components/router.h"
#include "greensocs/base-components/remote.h"

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
        , m_rpc_pass("remote_pass")
        , m_broker(cci::cci_get_broker())
        , m_gdb_port("gdb_port", 0, "GDB port")
        , m_qemu_inst(qemu_inst)
        , m_router("remote_router")
        , m_cpu("cpu", m_qemu_inst)
    {
        unsigned int m_irq_num = m_broker.get_param_handle(std::string(this->name()) + ".cpu.nvic.num_irq")
                                     .get_cci_value()
                                     .get_uint();

        if (!m_gdb_port.is_default_value()) m_cpu.p_gdb_port = m_gdb_port;

        if (m_irq_num > m_rpc_pass.p_initiator_signals_num)
            SCP_FATAL(()) << std::string(this->name()) << ".cpu.nvic.num_irq"
                          << "(" << m_irq_num << ") > RemoteCPU number of available signals ("
                          << m_rpc_pass.p_initiator_signals_num << ")!";

        SCP_INFO(()) << "number of irqs  = " << m_irq_num;

        for (unsigned int i = 0; i < m_irq_num; i++)
            m_rpc_pass.initiator_signal_sockets[i].bind(m_cpu.m_nvic.irq_in[i]);

        m_router.initiator_socket.bind(m_cpu.m_nvic.socket);
        m_cpu.socket.bind(m_router.target_socket);
        m_router.initiator_socket.bind(m_rpc_pass.target_sockets[0]);
    }

private:
    gs::PassRPC<> m_rpc_pass;
    cci::cci_broker_handle m_broker;
    cci::cci_param<int> m_gdb_port;
    QemuInstance& m_qemu_inst;
    gs::Router<> m_router;
    CpuArmCortexM55 m_cpu;
};

class RemotePlatform : public gs::ModuleFactory::Container
{
protected:
    cci::cci_param<int> p_quantum_ns;

public:
    RemotePlatform(const sc_core::sc_module_name& n)
        : gs::ModuleFactory::Container(n), p_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
    {
        using tlm_utils::tlm_quantumkeeper;

        SCP_DEBUG(()) << "Constructor";

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);
    }

    SCP_LOGGER(());

    ~RemotePlatform() {}
};

int sc_main(int argc, char* argv[])
{
    std::cout << "RemoteCPU started\n";
    std::cout << "Passed arguments: " << std::endl;
    for (int i = 0; i < 2; i++) std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    auto m_broker = new gs::ConfigurableBroker(
        argc, argv,
        { { "remote_platform.moduletype", cci::cci_value("Container") },
          { "remote_platform.quantum_ns", cci::cci_value(10000000) },
          { "remote_platform.qmeu_inst_mgr.moduletype", cci::cci_value("QemuInstanceManager") },
          { "remote_platform.qemu_inst.moduletype", cci::cci_value("QemuInstance") },
          { "remote_platform.qemu_inst.args.0", cci::cci_value("&remote_platform.qmeu_inst_mgr") },
          { "remote_platform.qemu_inst.args.1", cci::cci_value("AARCH64") },
          { "remote_platform.qemu_inst.sync_policy", cci::cci_value("multithread-freerunning") },
          { "remote_platform.remote_cpu.moduletype", cci::cci_value("RemoteCPU") },
          { "remote_platform.remote_cpu.args.0", cci::cci_value("&remote_platform.qemu_inst") },
          { "remote_platform.remote_cpu.remote_pass.tlm_target_ports_num", cci::cci_value(1) },
          { "remote_platform.remote_cpu.remote_pass.tlm_initiator_ports_num", cci::cci_value(0) },
          { "remote_platform.remote_cpu.remote_pass.initiator_signals_num", cci::cci_value(1) },
          { "remote_platform.remote_cpu.remote_pass.target_socket_0.address", cci::cci_value(0) },
          { "remote_platform.remote_cpu.remote_pass.target_socket_0.size",
            cci::cci_value(std::numeric_limits<uint64_t>::max()) },
          { "remote_platform.remote_cpu.cpu.nvic.mem.address", cci::cci_value(0xE000E000) },
          { "remote_platform.remote_cpu.cpu.nvic.mem.size", cci::cci_value(0x10000) },
          { "remote_platform.remote_cpu.cpu.nvic.num_irq", cci::cci_value(1) } });

    GSC_MODULE_REGISTER(RemoteCPU, sc_core::sc_object*);

    RemotePlatform remote("remote_platform");
    try {
        sc_core::sc_start();
    } catch (std::runtime_error const& e) {
        std::cerr << "Error: (Remote CPU )  '" << e.what() << "'\n";
        exit(1);
    } catch (...) {
        std::cerr << "Unknown error (Remote CPU)!\n";
        exit(2);
    }
    return 0;
}