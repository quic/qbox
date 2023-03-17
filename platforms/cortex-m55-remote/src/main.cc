/*
 *  M55 mini-vp
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

#include <chrono>
#include <string>

#include <cci_configuration>
#include <systemc>

#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/luafile_tool.h>

#include <libqbox/components/cpu/arm/cortex-m55.h>
#include <libqbox/components/uart/pl011.h>

#include <greensocs/base-components/memory.h>
#include <greensocs/base-components/router.h>
#include <greensocs/base-components/remote.h>
#include "greensocs/systemc-uarts/uart-pl011.h"
#include "greensocs/systemc-uarts/backends/char-backend.h"
#include <greensocs/systemc-uarts/backends/char/stdio.h>
#include <greensocs/systemc-uarts/backends/char/socket.h>

#define TLM_PORTS_NUM 1
#define SIGNALS_NUM   256

class CPUPassRPC : public sc_core::sc_module
{
public:
    CPUPassRPC(const sc_core::sc_module_name& p_name)
        : sc_core::sc_module(p_name)
        , argv(get_argv())
        , remote_exec_path("remote_exec_path", "", "Remote process executable path")
        , m_rpc_pass("local_pass", remote_exec_path.get_value(), argv) {}

    void bind_tlm_ports(gs::Router<>& router) {
        router.add_initiator(m_rpc_pass.initiator_sockets[0]);
    }

    template <class T>
    using InitiatorSignalSocket = sc_core::sc_port<sc_core::sc_signal_inout_if<T>, 1,
                                                   sc_core::SC_ZERO_OR_MORE_BOUND>;
    void bind_irq(InitiatorSignalSocket<bool> &irq, unsigned int irq_num) {
        irq.bind(m_rpc_pass.target_signal_sockets[irq_num]);
    }

private:
    std::vector<std::string> get_argv() {
        std::vector<std::string> argv;
        std::list<std::string> argv_cci_children = gs::sc_cci_children(
            (std::string(name()) + ".remote_argv").c_str());
        std::transform(argv_cci_children.begin(), argv_cci_children.end(), std::back_inserter(argv),
                       [this](std::string arg) {
                           std::string args = (std::string(name()) + ".remote_argv." + arg);
                           return gs::cci_get<std::string>(args + ".key") + "=" +
                                  gs::cci_get<std::string>(args + ".val");
                       });
        return argv;
    }

private:
    std::vector<std::string> argv;
    cci::cci_param<std::string> remote_exec_path;
    gs::PassRPC<TLM_PORTS_NUM, SIGNALS_NUM> m_rpc_pass;
};

class GreenSocsPlatform : public sc_core::sc_module
{
protected:
    gs::ConfigurableBroker m_broker;
    cci::cci_param<int> m_quantum_ns;
    CPUPassRPC m_remote;
    gs::Router<> m_router;
    gs::Memory<> m_ram;
    Uart m_uart;
    CharBackend* uart_backend;

    void setup_irq_mapping() {
        int irq = gs::cci_get<int>(std::string(m_uart.name()) + ".irq");
        m_remote.bind_irq(m_uart.irq, irq);
    }

public:
    GreenSocsPlatform(const sc_core::sc_module_name& n)
        : sc_core::sc_module(n)
        , m_broker({})
        , m_quantum_ns("quantum-ns", 1000000, "TLM-2.0 global quantum in ns")
        , m_remote("remote")
        , m_router("router")
        , m_ram("ram")
        , m_uart("uart")
        , uart_backend(new CharBackendStdio("backend_stdio")) {
        using tlm_utils::tlm_quantumkeeper;
        sc_core::sc_time global_quantum(m_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);

        setup_irq_mapping();

        m_remote.bind_tlm_ports(m_router);
        m_router.initiator_socket.bind(m_ram.socket);
        m_router.initiator_socket.bind(m_uart.socket);

        m_uart.set_backend(uart_backend);
    }
};

int sc_main(int argc, char* argv[]) {
    auto m_broker = new gs::ConfigurableBroker(argc, argv);

    GreenSocsPlatform platform("platform");

    auto start = std::chrono::system_clock::now();
    sc_core::sc_start();
    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds() << "SC_SEC"
              << std::endl;
    std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)" << std::endl;

    return 0;
}