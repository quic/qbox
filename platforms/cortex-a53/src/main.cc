/*
 *  Copyright (C) 2022 GreenSocs
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <chrono>
#include <string>

#include <cci_configuration>
#include <systemc>

#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/luafile_tool.h>

#include <libqbox/components/cpu/arm/cortex-a53.h>
#include <libqbox/components/irq-ctrl/arm-gicv3.h>
#include <libqbox/components/uart/pl011.h>

#include <greensocs/base-components/memory.h>
#include <greensocs/base-components/router.h>

#define ARCH_TIMER_VIRT_IRQ (16 + 11)
#define ARCH_TIMER_S_EL1_IRQ (16 + 13)
#define ARCH_TIMER_NS_EL1_IRQ (16 + 14)
#define ARCH_TIMER_NS_EL2_IRQ (16 + 10)

class GreenSocsPlatform : public sc_core::sc_module {
protected:
  cci::cci_param<unsigned> p_arm_num_cpus;
  cci::cci_param<unsigned> p_num_redists;

  gs::ConfigurableBroker m_broker;

  cci::cci_param<int> m_quantum_ns;
  cci::cci_param<int> m_gdb_port;

  QemuInstanceManager m_inst_mgr;
  QemuInstance &m_qemu_inst;

  sc_core::sc_vector<QemuCpuArmCortexA53> m_cpus;
  QemuArmGicv3 m_gic;
  gs::Router<> m_router;
  gs::Memory<> m_ram;
  gs::Memory<> m_flash;
  QemuUartPl011 m_uart;

  gs::Memory<> m_fallback_mem;

  gs::Loader<> m_loader;

  void do_bus_binding() {
    if (p_arm_num_cpus) {
      for (auto &cpu : m_cpus) {
        cpu.socket.bind(m_router.target_socket);
      }

      m_router.initiator_socket.bind(m_gic.dist_iface);
      m_router.initiator_socket.bind(m_gic.redist_iface[0]);
    }

    m_router.initiator_socket.bind(m_ram.socket);
    m_router.initiator_socket.bind(m_flash.socket);
    m_router.initiator_socket.bind(m_uart.socket);

    // General loader
    m_loader.initiator_socket.bind(m_router.target_socket);

    // MUST be added last
    m_router.initiator_socket.bind(m_fallback_mem.socket);
  }

  void setup_irq_mapping() {
    if (p_arm_num_cpus) {
      {
        int irq = gs::cci_get<int>(std::string(m_uart.name()) + ".irq");
        m_uart.irq_out.bind(m_gic.spi_in[irq]);
      }
      int i = 0;
      for (auto &cpu : m_cpus) {
        m_gic.irq_out[i].bind(cpu.irq_in);
        m_gic.fiq_out[i].bind(cpu.fiq_in);

        m_gic.virq_out[i].bind(cpu.virq_in);
        m_gic.vfiq_out[i].bind(cpu.vfiq_in);

        cpu.irq_timer_phys_out.bind(m_gic.ppi_in[i][ARCH_TIMER_NS_EL1_IRQ]);
        cpu.irq_timer_virt_out.bind(m_gic.ppi_in[i][ARCH_TIMER_VIRT_IRQ]);
        cpu.irq_timer_hyp_out.bind(m_gic.ppi_in[i][ARCH_TIMER_NS_EL2_IRQ]);
        cpu.irq_timer_sec_out.bind(m_gic.ppi_in[i][ARCH_TIMER_S_EL1_IRQ]);

        i++;
      }
    }
  }

public:
  GreenSocsPlatform(const sc_core::sc_module_name &n)
      : sc_core::sc_module(n),
        p_arm_num_cpus("arm_num_cpus", 4, "Number of ARM cores"),
        p_num_redists("num_redists", 1, "Number of redistribution regions"),
        m_broker({
            {"gic.num_spi", cci::cci_value(64)},
            {"gic.redist_region",
             cci::cci_value(std::vector<unsigned int>(
                 p_num_redists, p_arm_num_cpus / p_num_redists))},
        }),
        m_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns"),
        m_gdb_port("gdb_port", 0, "GDB port")

        ,
        m_qemu_inst(m_inst_mgr.new_instance(QemuInstance::Target::AARCH64)),
        m_cpus("cpu", p_arm_num_cpus,
               [this](const char *n, size_t i) {
                 return new QemuCpuArmCortexA53(n, m_qemu_inst);
               }),
        m_gic("gic", m_qemu_inst, p_arm_num_cpus), m_router("router"),
        m_ram("ram"), m_flash("flash"), m_uart("uart", m_qemu_inst),
        m_fallback_mem("fallback_memory"), m_loader("load") {
    using tlm_utils::tlm_quantumkeeper;

    sc_core::sc_time global_quantum(m_quantum_ns, sc_core::SC_NS);
    tlm_quantumkeeper::set_global_quantum(global_quantum);

    do_bus_binding();
    setup_irq_mapping();
  }
};

int sc_main(int argc, char *argv[]) {
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));      // make the msg type column a bit tighter

  auto m_broker = new gs::ConfigurableBroker(argc, argv);

  GreenSocsPlatform platform("platform");

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
  std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds()
            << "SC_SEC" << std::endl;
  std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)"
            << std::endl;

  return 0;
}
