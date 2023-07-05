/*
 * Copyright (c) 2022 GreenSocs
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

#define GS_USE_TARGET_MODULE_FACTORY

#include <chrono>
#include <string>
#include <getopt.h>

#include <systemc>
#include <cci_configuration>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <greensocs/gsutils/luafile_tool.h>
#include <greensocs/gsutils/cciutils.h>
#include <greensocs/libgsutils.h>
#include <greensocs/gsutils/module_factory_container.h>

#include <libqbox/components/cpu/arm/cortex-a76.h>
#include <libqbox/components/cpu/hexagon/hexagon.h>
#include <libqbox/components/irq-ctrl/arm-gicv3.h>
#include <libqbox/components/uart/pl011.h>
#include <libqbox/components/irq-ctrl/hexagon-l2vic.h>
#include <libqbox/components/timer/hexagon-qtimer.h>
#include <libqbox/components/net/virtio-mmio-net.h>
#include <libqbox/components/blk/virtio-mmio-blk.h>
#include <libqbox/components/mmu/arm-smmu.h>
#include <libqbox/qemu-instance.h>

#include <libqbox-extra/components/meta/global_peripheral_initiator.h>
#include <libqbox-extra/components/pci/gpex.h>
#include <libqbox-extra/components/pci/rtl8139.h>
#include <libqbox-extra/components/pci/virtio-gpu-pci.h>
#include <libqbox-extra/components/pci/virtio-gpu-gl-pci.h>
#include <libqbox-extra/components/pci/nvme.h>
#include <libqbox-extra/components/pci/xhci.h>
#include <libqbox-extra/components/display/display.h>
#include <libqbox-extra/components/usb/kbd.h>
#include <libqbox-extra/components/usb/tablet.h>

#include <greensocs/base-components/router.h>
#include <greensocs/base-components/memory.h>
#include <greensocs/base-components/pass.h>
#include <greensocs/base-components/memorydumper.h>
#include <greensocs/base-components/remote.h>
#include "greensocs/systemc-uarts/uart-pl011.h"
#include "greensocs/systemc-uarts/backends/char-backend.h"
#include <greensocs/systemc-uarts/backends/char/stdio.h>
#include <greensocs/systemc-uarts/backends/char-bf/stdio.h>
#include <greensocs/systemc-uarts/backends/char/socket.h>

#include <quic/ipcc/ipcc.h>
#include <quic/qtb/qtb.h>
#include <quic/csr/csr.h>
#include <quic/mpm/mpm.h>
#include <quic/smmu500/smmu500.h>
#include <quic/qup/uart/uart-qupv3.h>
#include <quic/qupv3_qupv3_se_wrapper_se0/qupv3_qupv3_se_wrapper_se0.h>
#include <quic/gen_boilerplate/reg_model_maker.h>

#include "wdog.h"
#include "pll.h"

#define HEXAGON_CFGSPACE_ENTRIES    (128)
#define HEXAGON_CFG_ADDR_BASE(addr) ((addr >> 16) & 0x0fffff)

#define ARCH_TIMER_VIRT_IRQ   (16 + 11)
#define ARCH_TIMER_S_EL1_IRQ  (16 + 13)
#define ARCH_TIMER_NS_EL1_IRQ (16 + 14)
#define ARCH_TIMER_NS_EL2_IRQ (16 + 10)

#define PASSRPC_TLM_PORTS_NUM 20
#define PASSRPC_SIGNALS_NUM   20
#define newsmmu

class hexagon_cluster : public sc_core::sc_module
{
public:
    cci::cci_param<unsigned> p_hexagon_num_threads;
    cci::cci_param<uint32_t> p_hexagon_start_addr;
    cci::cci_param<bool> p_isdben_secure;
    cci::cci_param<bool> p_isdben_trusted;
    cci::cci_param<uint32_t> p_cfgbase;
    cci::cci_param<bool> p_vp_mode;
    // keep public so that smmu can be associated with hexagon
    QemuInstance& m_qemu_hex_inst;

private:
    QemuHexagonL2vic m_l2vic;
    QemuHexagonQtimer m_qtimer;
    WDog<> m_wdog;
    sc_core::sc_vector<pll<>> m_plls;
    //    pll<> m_pll2;
    csr m_csr;
    gs::pass<> m_pass;
    gs::Memory<> m_rom;

    sc_core::sc_vector<QemuCpuHexagon> m_hexagon_threads;
    GlobalPeripheralInitiator m_global_peripheral_initiator_hex;

public:
    // expose the router, to facilitate binding
    // otherwise we would need a 'bridge' to bind to the router and an
    // initiator socket.
    gs::Router<> m_router;
    hexagon_cluster(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
    : hexagon_cluster(name, *(dynamic_cast<QemuInstance*>(o)), *(dynamic_cast<gs::Router<>*>(t)))
    {
    }
    hexagon_cluster(const sc_core::sc_module_name& n, QemuInstance& qemu_hex_inst,
                    gs::Router<>& parent_router)
        : sc_core::sc_module(n)
        , p_hexagon_num_threads("hexagon_num_threads", 8, "Number of Hexagon threads")
        , p_hexagon_start_addr("hexagon_start_addr", 0x100, "Hexagon execution start address")
        , p_isdben_trusted("isdben_trusted", false, "Value of ISDBEN.TRUSTED reg field")
        , p_isdben_secure("isdben_secure", false, "Value of ISDBEN.SECURE reg field")
        , p_cfgbase("cfgtable_base", 0, "config table base address")
        , p_vp_mode("vp_mode", true, "override the vp_mode for testing")
        // , m_qemu_hex_inst(
        //       m_inst_mgr.new_instance("HexagonQemuInstance", QemuInstance::Target::HEXAGON))
        , m_qemu_hex_inst(qemu_hex_inst)
        , m_l2vic("l2vic", m_qemu_hex_inst)
        , m_qtimer("qtimer",
                   m_qemu_hex_inst) // are we sure it's in the hex cluster?????
        , m_wdog("wdog")
        , m_plls("pll", 1)
        //        , m_pll2("pll2")
        , m_csr("csr")
        , m_rom("rom")
        , m_pass("pass", false)
        , m_hexagon_threads("hexagon_thread", p_hexagon_num_threads,
                            [this](const char* n, size_t i) {
                                /* here n is already "hexagon-cpu_<vector-index>" */
                                uint64_t l2vic_base = gs::cci_get<uint64_t>(cci::cci_get_broker(), std::string(name()) +
                                                                            ".l2vic.mem.address");
                                                        
                                uint64_t qtmr_rg0 = gs::cci_get<uint64_t>(cci::cci_get_broker(),
                                    std::string(name()) + ".qtimer.mem_view.address");
                                return new QemuCpuHexagon(
                                    n, m_qemu_hex_inst, p_cfgbase, QemuCpuHexagon::v68_rev,
                                    l2vic_base, qtmr_rg0, p_hexagon_start_addr, p_vp_mode);
                            })
        , m_router("router")
        , m_global_peripheral_initiator_hex("glob-per-init-hex", qemu_hex_inst,
                                            m_hexagon_threads[0]) {
        parent_router.initiator_socket.bind(m_l2vic.socket);
        parent_router.initiator_socket.bind(m_l2vic.socket_fast);
        parent_router.initiator_socket.bind(m_qtimer.socket);
        parent_router.initiator_socket.bind(m_qtimer.view_socket);
        parent_router.initiator_socket.bind(m_wdog.socket);
        for (auto& pll : m_plls) {
            parent_router.initiator_socket.bind(pll.socket);
        }
        parent_router.initiator_socket.bind(m_rom.socket);

        parent_router.initiator_socket.bind(m_csr.socket);
        m_csr.hex_halt.bind(m_hexagon_threads[0].halt);

        // pass through transactions.
        m_router.initiator_socket.bind(m_pass.target_socket);
        m_pass.initiator_socket.bind(parent_router.target_socket);

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
    }
};

class GreenSocsPlatform : public gs::ModuleFactory::Container
{
protected:

    cci::cci_param<int> p_quantum_ns;

public:
    GreenSocsPlatform(const sc_core::sc_module_name& n)
        : gs::ModuleFactory::Container(n)
        , p_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
        {
        using tlm_utils::tlm_quantumkeeper;

        SCP_DEBUG(()) << "Constructor";

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);

        // UNCOMMENT
        // if (p_with_gpu.get_value()) {
        //     for (auto& gpex : m_gpex){
        //         m_gpu = new QemuVirtioGpuGlPci("gpu", m_qemu_inst);
        //         gpex.add_device(*m_gpu);
        //     // m_display = new QemuDisplay("display", *m_gpu);
        //     }
        // }

    }

    SCP_LOGGER(());

    ~GreenSocsPlatform() {
    }
};

static void parse_local_args(int argc, char* argv[]) {
    // getopt permutes argv array, so copy it to argv_cp
    const int BUFFER_SIZE = 8192;
    char argv_buffer[BUFFER_SIZE];
    char* buffer_p = argv_buffer;
    char** argv_cp = new char*[argc];
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]) + 1; // count \0
        strcpy(buffer_p, argv[i]);
        argv_cp[i] = buffer_p;
        buffer_p += len;
    }

    // configure getopt
    optind = 0; // reset of getopt
    opterr = 0; // avoid error message for not recognized option
    static const char* optstring = "hv";
    static struct option long_options[] = { { "help", no_argument, 0, 'h' },
                                            { "version", no_argument, 0, 'v' },
                                            { 0, 0, 0, 0 } };
    while (1) {
        int c = getopt_long(argc, argv_cp, optstring, long_options, 0);
        if (c == EOF)
            break;
        if (c == 'h') {
            std::cout << "\nPossible Options/Arguments:\n"
                         "\n"
                         "      --gs_luafile <filename>\n"
                         "        execute a Lua script and loads all the globals as\n"
                         "        parameters [required]\n"
                         "\n"
                         "      --param <param_name=value>\n"
                         "        set param name (foo.baa) to value\n"
                         "\n"
                         "      --debug\n"
                         "        shows the state of the configurable parameters at\n"
                         "        the beginning of the simulation and halts.\n"
                         "\n"
                         "      --version\n"
                         "        displays the qqvp version\n"
                         "\n"
                         "      --help\n"
                         "        this help\n"
                         "\n"
                         "      Any extra arguments will be treated as lua config\n"
                         "      files. That is, as --gs_luafile arguments.\n"
                         "\n"
                      << std::flush;
            exit(0);
        } else if (c == 'v') {
            std::cout << "\nQualcomm QEMU-Virtual Platform " << QQVP_VERSION << "\n"
                      << std::flush;
            exit(0);
        }
    }

    delete[] argv_cp;
}

int sc_main(int argc, char* argv[]) {
    parse_local_args(argc, argv);
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));      // make the msg type column a bit tighter

    gs::ConfigurableBroker m_broker(argc, argv);
    cci::cci_originator orig("sc_main");
    cci::cci_param<int> p_log_level{ "log_level", 0, "Default log level", cci::CCI_ABSOLUTE_NAME, orig };
    cci::cci_param<std::string> p_zipfile{ "zipfile", "", "Default log level", cci::CCI_ABSOLUTE_NAME, orig };

    gs::json_zip_archive jza(zip_open(p_zipfile.get_value().c_str(), ZIP_RDONLY, nullptr));

    jza.json_read_cci(m_broker.create_broker_handle(orig), "platform");



    typedef gs::PassRPC<PASSRPC_TLM_PORTS_NUM, PASSRPC_SIGNALS_NUM> PassRPC;

    // GSC_MODULE_REGISTER(PassRPC);
    GSC_MODULE_REGISTER(hexagon_cluster, sc_core::sc_object*, sc_core::sc_object*);

    GreenSocsPlatform platform("platform");

    auto start = std::chrono::system_clock::now();
    try {
        SCP_INFO() << "SC_START";
        sc_core::sc_start();
    } catch (std::runtime_error const& e) {
        std::cerr << argv[0] << "Error: '" << e.what() << std::endl;
        exit(1);
    } catch (const std::exception& exc) {
        std::cerr << argv[0] << " Error: '" << exc.what() << std::endl;
        exit(2);
    } catch (...) {
        SCP_ERR() << "Unknown error (main.cc)!";
        exit(3);
    }

    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds() << "SC_SEC"
              << std::endl;
    std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)" << std::endl;

    return 0;
}
