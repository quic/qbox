/*
 *  GreenSocs base virtual platform
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Luc Michel
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

#include <systemc>
#include <cci/utils/broker.h>

#include <greensocs/gsutils/luafile_tool.h>
#include <greensocs/gsutils/cciutils.h>

#include "libqbox/components/cpu/riscv64/riscv64.h"
#include "libqbox/components/irq-ctrl/plic-sifive.h"
#include "libqbox/components/irq-ctrl/riscv-aclint-swi.h"
#include "libqbox/components/timer/riscv-aclint-mtimer.h"
#include "libqbox/components/uart/16550.h"

#include <greensocs/base-components/router.h>
#include <greensocs/base-components/memory.h>

#include <stdio.h>

class RiscvDemoPlatform : public sc_core::sc_module {

protected:
    cci::cci_param<unsigned> p_riscv_num_cpus;
    cci::cci_param<int> p_log_level;

    gs::ConfigurableBroker m_broker;

    cci::cci_param<int> m_quantum_ns;
    cci::cci_param<int> m_gdb_port;

    QemuInstanceManager m_inst_mgr;
    QemuInstance &m_qemu_inst;
    sc_core::sc_vector<QemuCpuRiscv64Rv64> m_cpus;
    QemuRiscvSifivePlic m_plic;
    QemuRiscvAclintSwi m_swi;
    QemuRiscvAclintMtimer m_mtimer;

    gs::Router<> m_router;

    /*
     * QEMU RISC-V cpu boots at 0x1000. We put a small rom here to jump into
     * the bootrom.
     */
    gs::Memory<> m_resetvec_rom;

    gs::Memory<> m_rom;
    gs::Memory<> m_sram;
    gs::Memory<> m_dram;

    /* Modeled as memory */
    gs::Memory<> m_qspi;
    gs::Memory<> m_boot_gpio;

    QemuUart16550 m_uart;

    gs::Memory<> m_fallback_mem;

    gs::Loader<> m_loader;

    void do_bus_binding()
    {
        if (p_riscv_num_cpus) {
            for (auto& cpu : m_cpus) {
                cpu.socket.bind(m_router.target_socket);
            }
        }

        m_router.initiator_socket.bind(m_resetvec_rom.socket);
        m_router.initiator_socket.bind(m_rom.socket);
        m_router.initiator_socket.bind(m_sram.socket);
        m_router.initiator_socket.bind(m_dram.socket);
        m_router.initiator_socket.bind(m_qspi.socket);
        m_router.initiator_socket.bind(m_boot_gpio.socket);
        m_router.initiator_socket.bind(m_plic.socket);
        m_router.initiator_socket.bind(m_swi.socket);
        m_router.initiator_socket.bind(m_mtimer.socket);
        m_router.initiator_socket.bind(m_uart.socket);
        

        // General loader
        m_loader.initiator_socket.bind(m_router.target_socket);

        // MUST be added last
        m_router.initiator_socket.bind(m_fallback_mem.socket);
    }

    void setup_irq_mapping() {
        if (p_riscv_num_cpus)
        {
            int irq = gs::cci_get<int>(cci::cci_get_broker(), std::string(m_uart.name()) + ".irq");
            m_uart.irq_out.bind(m_plic.irq_in[irq]);
        }
    }

public:
    RiscvDemoPlatform(const sc_core::sc_module_name &n)
        : sc_core::sc_module(n)
        , p_riscv_num_cpus("riscv_num_cpus", 5, "Number of RISCV cores")
        , p_log_level("log_level", 2, "Default log level")
        , m_broker({
                   {"plic.num_sources", cci::cci_value(280)},
                   {"plic.num_priorities", cci::cci_value(7)},
                   {"plic.priority_base", cci::cci_value(0x04)},
                   {"plic.pending_base", cci::cci_value(0x1000)},
                   {"plic.enable_base", cci::cci_value(0x2000)},
                   {"plic.enable_stride", cci::cci_value(0x80)},
                   {"plic.context_base", cci::cci_value(0x200000)},
                   {"plic.context_stride", cci::cci_value(0x1000)},
                   {"plic.aperture_size", cci::cci_value(0x4000000)},
                   {"plic.hart_config", cci::cci_value("MS,MS,MS,MS,MS")},

                   {"uart.regshift", cci::cci_value(2)},
                   {"uart.baudbase", cci::cci_value(38400000)},

                   {"swi.num_harts", cci::cci_value(5)},

                   {"mtimer.timecmp_base", cci::cci_value(0x0)},
                   {"mtimer.time_base", cci::cci_value(0xbff8)},
                   {"mtimer.provide_rdtime", cci::cci_value(true)},
                   {"mtimer.aperture_size", cci::cci_value(0x10000)},
                   {"mtimer.num_harts", cci::cci_value(5)},
                   {"mtimer.timebase_freq", cci::cci_value{1000000}},
        })
        , m_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
        , m_gdb_port("gdb_port", 0, "GDB port")

        , m_qemu_inst(m_inst_mgr.new_instance("RISCVQemuInstance", QemuInstance::Target::RISCV64))
        , m_cpus("cpu", p_riscv_num_cpus, [this] (const char *n, size_t i) { return new QemuCpuRiscv64Rv64(n, m_qemu_inst, i); })
        , m_plic("plic", m_qemu_inst)
        , m_swi("swi", m_qemu_inst)
        , m_mtimer("mtimer", m_qemu_inst)
        , m_router("router")
        , m_resetvec_rom("resetvec_rom")
        , m_rom("rom")
        , m_sram("sram")
        , m_dram("dram")
        , m_qspi("qspi")
        , m_boot_gpio("boot_gpio")
        , m_uart("uart", m_qemu_inst)
        , m_fallback_mem("fallback_memory")
        , m_loader("load")
    {
        using tlm_utils::tlm_quantumkeeper;

        int level = p_log_level.get_value();
        scp::set_logging_level(scp::as_log(level));

        sc_core::sc_time global_quantum(m_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);

        do_bus_binding();
        setup_irq_mapping();
    }
};

int sc_main(int argc, char *argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));      // make the msg type column a bit tighter

    auto m_broker = new gs::ConfigurableBroker(argc, argv);
  
    RiscvDemoPlatform platform("platform");

    auto start = std::chrono::system_clock::now();
    try {
        SCP_INFO() << "SC_START";
        sc_core::sc_start();
    } catch (std::runtime_error const &e) {
        std::cerr << argv[0] << "Error: '" << e.what() << std::endl;
        exit(1);
    } catch (const std::exception &exc) {
        std::cerr << argv[0] << " Error: '" << exc.what() << std::endl;
        exit(2);
    } catch (...) {
        SCP_ERR() << "Unknown error (main.cc)!";
        exit(3);
    }

    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds() << "SC_SEC" << std::endl;
    std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)" << std::endl;

    return 0;
}
