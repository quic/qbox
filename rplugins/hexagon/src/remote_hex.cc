/*
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
#include <libqbox/components/cpu/hexagon/hexagon.h>
#include <libqbox/ports/initiator-signal-socket.h>
#include <libqbox-extra/components/meta/global_peripheral_initiator.h>
#include <libqbox/components/irq-ctrl/hexagon-l2vic.h>
#include <libqbox/components/timer/hexagon-qtimer.h>
#include "greensocs/base-components/memory.h"
#include "greensocs/base-components/remote.h"
#include "greensocs/base-components/router.h"
#include <quic/csr/csr.h>
#include "../../../src/wdog.h"
#include "../../../src/pll.h"

class RemoteHex : public sc_core::sc_module
{
    gs::PassRPC<3, 1> m_rpass; // for the l2vic IRQ

public:
    cci::cci_param<unsigned> p_hexagon_num_threads;
    cci::cci_param<uint32_t> p_hexagon_start_addr;
    cci::cci_param<uint32_t> p_cfgbase;
    cci::cci_param<int> p_quantum_ns;

private:
    QemuInstanceManager m_inst_mgr;
    QemuInstance& m_qemu_hex_inst;
    QemuHexagonL2vic m_l2vic;
    QemuHexagonQtimer m_qtimer;
    sc_core::sc_vector<QemuCpuHexagon> m_hexagon_threads;
    GlobalPeripheralInitiator m_global_peripheral_initiator_hex;
    WDog<> m_wdog;
    sc_core::sc_vector<pll<>> m_plls;
    csr m_csr;
    gs::Memory<> m_rom;
    gs::Memory<> m_fallback_mem;
    gs::Router<> m_router;

public:
    RemoteHex(const sc_core::sc_module_name& n)
        : sc_core::sc_module(n)
        , m_rpass("remote")
        , p_hexagon_num_threads("hexagon_num_threads", 8, "Number of Hexagon threads")
        , p_hexagon_start_addr("hexagon_start_addr", 0x100, "Hexagon execution start address")
        , p_cfgbase("cfgtable_base", 0, "config table base address")
        , p_quantum_ns("quantum_ns", 10000000, "TLM-2.0 global quantum in ns")
        , m_qemu_hex_inst(
              m_inst_mgr.new_instance("HexagonQemuInstance", QemuInstance::Target::HEXAGON))
        , m_l2vic("l2vic", m_qemu_hex_inst)
        , m_qtimer("qtimer",
                   m_qemu_hex_inst) // are we sure it's in the hex cluster?????
        , m_hexagon_threads("hexagon_thread", p_hexagon_num_threads,
                            [this](const char* n, size_t i) {
                                /* here n is already "hexagon-cpu_<vector-index>" */
                                uint64_t l2vic_base = gs::cci_get<uint64_t>(cci::cci_get_broker(), std::string(name()) +
                                                                            ".l2vic.mem.address");
                                uint64_t qtmr_rg0 = gs::cci_get<uint64_t>(cci::cci_get_broker(),
                                    std::string(name()) + ".qtimer.mem_view.address");
                                return new QemuCpuHexagon(n, m_qemu_hex_inst, p_cfgbase,
                                                          QemuCpuHexagon::v68_rev, l2vic_base,
                                                          qtmr_rg0, p_hexagon_start_addr);
                            })
        , m_global_peripheral_initiator_hex("glob-per-init-hex", m_qemu_hex_inst,
                                            m_hexagon_threads[0])
        , m_wdog("wdog")
        , m_plls("pll", 4)
        , m_csr("csr")
        , m_rom("rom")
        , m_fallback_mem("fallback_memory")
        , m_router("router")
         {
        /* Set global quantum */
        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_utils::tlm_quantumkeeper::set_global_quantum(global_quantum);

        int is = 0, ts = 0;

        m_rpass.initiator_sockets[is++].bind(
            m_router.target_socket); // for traffic from outside comming in.
        m_rpass.initiator_sockets[is++].bind(
            m_router.target_socket); // for traffic from outside comming in.
        //m_rpass.initiator_sockets[is++].bind(
        //    m_router.target_socket); // for traffic from outside comming in.

        m_router.initiator_socket.bind(m_l2vic.socket);
        m_router.initiator_socket.bind(m_l2vic.socket_fast);
        m_router.initiator_socket.bind(m_qtimer.socket);
        m_router.initiator_socket.bind(m_qtimer.view_socket);
        m_router.initiator_socket.bind(m_wdog.socket);
        for (auto& pll : m_plls) {
            m_router.initiator_socket.bind(pll.socket);
        }
        m_router.initiator_socket.bind(m_rom.socket);

        m_router.initiator_socket.bind(m_csr.socket);

        m_csr.hex_halt.bind(m_hexagon_threads[0].halt);

        for (auto& cpu : m_hexagon_threads) {
            cpu.socket.bind(m_router.target_socket);
        }
        for (int i = 0; i < m_l2vic.p_num_outputs; ++i) {
            m_l2vic.irq_out[i].bind(m_hexagon_threads[0].irq_in[i]);
        }

        m_qtimer.irq[0].bind(m_l2vic.irq_in[2]); // FIXME: Depends on static boolean
                                                 // syscfg_is_linux, may be 2
        m_qtimer.irq[1].bind(m_l2vic.irq_in[3]);

        m_global_peripheral_initiator_hex.m_initiator.bind(m_router.target_socket);

        m_rpass.initiator_signal_sockets[0].bind(m_l2vic.irq_in[30]);

        m_router.initiator_socket.bind(m_rpass.target_sockets[ts++]); // for SMMU traffic
        m_router.initiator_socket.bind(m_rpass.target_sockets[ts++]); // for non SMMU traffic
        m_router.initiator_socket.bind(m_rpass.target_sockets[ts++]); // rest of non SMMU traffic

        m_router.initiator_socket.bind(
            m_fallback_mem.socket); // fallback for all other regions inside the hex cluster
    }
};

int sc_main(int argc, char* argv[]) {
    std::cout << "Remote started\n";
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));      // make the msg type column a bit tighter
    auto m_broker = new gs::ConfigurableBroker(argc, argv,
                                               {
                                                   { "log_level", cci::cci_value(1) },
                                               });
    RemoteHex remote("remote_hex");
    try {
        sc_core::sc_start();
    } catch (std::runtime_error const& e) {
        std::cerr << "Remote HEX Error: '" << e.what() << "'\n";
        exit(1);
    } catch (const std::exception& exc) {
        std::cerr << "Remote hex Error: '" << exc.what() << "'\n";
        exit(2);
    } catch (...) {
        std::cerr << "Unknown error (Remote Hex)!\n";
        exit(3);
    }
    exit(EXIT_SUCCESS);
}