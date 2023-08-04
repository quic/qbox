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

#include <chrono>
#include <string>
#include <getopt.h>

#include <systemc>
#include <cci_configuration>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <greensocs/gsutils/luafile_tool.h>
#include <greensocs/gsutils/cciutils.h>

#include <libqbox/components/cpu/arm/cortex-a76.h>
#include <libqbox/components/cpu/hexagon/hexagon.h>
#include <libqbox/components/irq-ctrl/arm-gicv3.h>
#include <libqbox/components/uart/pl011.h>
#include <libqbox/components/irq-ctrl/hexagon-l2vic.h>
#include <libqbox/components/timer/hexagon-qtimer.h>
#include <libqbox/components/net/virtio-mmio-net.h>
#include <libqbox/components/blk/virtio-mmio-blk.h>
#include <libqbox/components/mmu/arm-smmu.h>

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
#include <greensocs/systemc-uarts/backends/char/socket.h>

#include <quic/ipcc/ipcc.h>
#include <quic/qtb/qtb.h>
#include <quic/csr/csr.h>
#include <quic/mpm/mpm.h>
#include <quic/smmu500/smmu500.h>
#include <quic/qup/uart/uart-qupv3.h>

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

    hexagon_cluster(const sc_core::sc_module_name& n, QemuInstanceManager& m_inst_mgr,
                    gs::Router<>& parent_router)
        : sc_core::sc_module(n)
        , p_hexagon_num_threads("hexagon_num_threads", 8, "Number of Hexagon threads")
        , p_hexagon_start_addr("hexagon_start_addr", 0x100, "Hexagon execution start address")
        , p_isdben_trusted("isdben_trusted", false, "Value of ISDBEN.TRUSTED reg field")
        , p_isdben_secure("isdben_secure", false, "Value of ISDBEN.SECURE reg field")
        , p_cfgbase("cfgtable_base", 0, "config table base address")
        , p_vp_mode("vp_mode", true, "override the vp_mode for testing")
        , m_qemu_hex_inst(
              m_inst_mgr.new_instance("HexagonQemuInstance", QemuInstance::Target::HEXAGON))
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
                                uint64_t l2vic_base = gs::cci_get<uint64_t>(std::string(name()) +
                                                                            ".l2vic.mem.address");
                                uint64_t qtmr_rg0 = gs::cci_get<uint64_t>(
                                    std::string(name()) + ".qtimer.mem_view.address");
                                return new QemuCpuHexagon(
                                    n, m_qemu_hex_inst, p_cfgbase, QemuCpuHexagon::v68_rev,
                                    l2vic_base, qtmr_rg0, p_hexagon_start_addr, p_vp_mode);
                            })
        , m_router("router")
        , m_global_peripheral_initiator_hex("glob-per-init-hex", m_qemu_hex_inst,
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

/*
 ** Plugin for remote connected models.
 ** FIXME: for now smmu500 is the only model which can be
 ** configured from a conf.lua and added to the local PassRPC
 ** hierarchy. The plugin should be more generic in future.
 */
#ifdef newsmmu
class SMMUConnectedPassRPC : public sc_core::sc_module
{
public:
    SMMUConnectedPassRPC(const sc_core::sc_module_name& p_name, smmu500<>* p_smmu)
        : sc_core::sc_module(p_name)
        , m_irqs_num(gs::sc_cci_list_items(name(), "irq").size())
        , m_smmu500_tbus_num(gs::sc_cci_list_items(name(), "smmu500_tbu").size())
        , p_num_initiators("num_initiators", m_smmu500_tbus_num, "Number of initiator ports")
        , p_num_targets("num_targets", 1, "Number of target ports")
        , argv(get_argv())
        , remote_exec_path("remote_exec_path", "", "Remote process executable path")
        , m_smmu500_tbus("smmu500_tbu", m_smmu500_tbus_num,
                         [&](const char* n, size_t i) { return new smmu500_tbu<>(n, p_smmu); })
        , m_remote("local_pass", remote_exec_path.get_value(), argv) {
        assert((p_num_initiators <= PASSRPC_TLM_PORTS_NUM) &&
               (p_num_initiators >= m_smmu500_tbus_num) &&
               (p_num_targets <= PASSRPC_TLM_PORTS_NUM) && (p_num_targets >= 0));
    }

    void bind_tlm_ports(gs::Router<>& router) {
        for (int i = 0; i < p_num_initiators.get_value(); i++) {
            if (i < m_smmu500_tbus_num) {
                m_remote.initiator_sockets[i].bind(m_smmu500_tbus[i].upstream_socket);
                m_smmu500_tbus[i].downstream_socket.bind(router.target_socket);
            } else {
                m_remote.initiator_sockets[i].bind(router.target_socket);
            }
        }
        for (int t = 0; t < p_num_targets.get_value(); t++) {
            router.initiator_socket.bind(m_remote.target_sockets[t]);
        }
    }

    void bind_irqs(QemuArmGicv3* gic) {
        for (uint32_t i = 0; i < m_irqs_num; ++i) {
            int irq = gs::cci_get<int>(std::string(this->name()) + ".irq_" + std::to_string(i));
            m_remote.initiator_signal_sockets[i].bind(gic->spi_in[irq]);
        }
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
    uint32_t m_irqs_num;
    uint32_t m_smmu500_tbus_num;
    cci::cci_param<unsigned> p_num_initiators;
    cci::cci_param<unsigned> p_num_targets;
    std::vector<std::string> argv;
    cci::cci_param<std::string> remote_exec_path;
    sc_core::sc_vector<smmu500_tbu<>> m_smmu500_tbus;

public:
    gs::PassRPC<PASSRPC_TLM_PORTS_NUM, PASSRPC_SIGNALS_NUM> m_remote;
};
#endif

class GreenSocsPlatform : public sc_core::sc_module
{
protected:
    cci::cci_param<unsigned> p_arm_num_cpus;
    cci::cci_param<unsigned> p_num_redists;
    cci::cci_param<unsigned> p_hexagon_num_clusters;

    cci::cci_param<bool> p_with_gpu;

    gs::ConfigurableBroker m_broker;

    cci::cci_param<int> p_quantum_ns;
    cci::cci_param<int> p_uart_backend;

    QemuInstanceManager m_inst_mgr;
    QemuInstanceManager m_inst_mgr_h;
    QemuInstance& m_qemu_inst;

    gs::Router<> m_router;
    sc_core::sc_vector<QemuCpuArmCortexA76> m_cpus;
    sc_core::sc_vector<hexagon_cluster> m_hexagon_clusters;
    QemuArmGicv3* m_gic;
    sc_core::sc_vector<gs::Memory<>> m_rams;
    sc_core::sc_vector<gs::Memory<>> m_hexagon_rams;
    //    gs::Memory<> m_system_imem;
    GlobalPeripheralInitiator* m_global_peripheral_initiator_arm;
    Uart* m_uart = nullptr;
    sc_core::sc_vector<QUPv3> m_uart_qups;

    IPCC m_ipcc;
    mpm_control m_mpm;
#ifdef newsmmu
    sc_core::sc_vector<smmu500<>> m_smmus;
    sc_core::sc_vector<smmu500_tbu<>> m_tbus;
#else
    QemuArmSmmu* m_smmu = nullptr;
#endif

    QemuVirtioMMIONet m_virtio_net_0;
    sc_core::sc_vector<QemuVirtioMMIOBlk> m_virtio_blks;

    QemuGPEX* m_gpex;
    sc_core::sc_vector<QemuVirtioGpu> m_gpus;
    QemuDisplay* m_display = nullptr;
    sc_core::sc_vector<QemuHexagonQtimer> m_qtimers;
    QemuRtl8139Pci* m_rtl8139;

    std::unique_ptr<QemuXhci> m_xhci;
    std::unique_ptr<QemuKbd> m_kbd;
    std::unique_ptr<QemuTablet> m_tablet;

    gs::Memory<> m_fallback_mem;

    gs::MemoryDumper<> m_memorydumper;
    gs::Loader<> m_loader;

    qtb<>* m_qtb;

#ifdef newsmmu
    sc_core::sc_vector<SMMUConnectedPassRPC> m_smmu_connd_remotes;
#endif

    void do_bus_binding() {
        if (p_arm_num_cpus) {
            for (auto& cpu : m_cpus) {
                cpu.socket.bind(m_router.target_socket);
            }

            m_router.initiator_socket.bind(m_gic->dist_iface);
            m_router.initiator_socket.bind(m_gic->redist_iface[0]);
            m_global_peripheral_initiator_arm->m_initiator.bind(m_router.target_socket);
        }

        for (auto& ram : m_rams) {
            m_router.initiator_socket.bind(ram.socket);
        }
        for (auto& ram : m_hexagon_rams) {
            m_router.initiator_socket.bind(ram.socket);
        }
        if (m_uart)
            m_router.initiator_socket.bind(m_uart->socket);
        m_router.initiator_socket.bind(m_ipcc.socket);
        m_router.initiator_socket.bind(m_mpm.socket);
        for (auto& qup : m_uart_qups) {
            m_router.initiator_socket.bind(qup.socket);
        }
#ifdef newsmmu
        for (auto& smmu : m_smmus) {
            m_router.initiator_socket.bind(smmu.socket);
        }
#endif
        m_router.initiator_socket.bind(m_virtio_net_0.socket);

        for (auto& vblk : m_virtio_blks) {
            m_router.initiator_socket.bind(vblk.socket);
        }

        m_router.initiator_socket.bind(m_gpex->ecam_iface);
        m_router.initiator_socket.bind(m_gpex->mmio_iface);
        m_router.initiator_socket.bind(m_gpex->mmio_iface_high);
        m_router.initiator_socket.bind(m_gpex->pio_iface);

        m_router.add_initiator(m_gpex->bus_master);

        for (auto& qt : m_qtimers) {
            m_router.initiator_socket.bind(qt.socket);
            m_router.initiator_socket.bind(qt.view_socket);
        }

        //        m_router.initiator_socket.bind(m_system_imem.socket);
    }

    void setup_irq_mapping() {
        if (p_arm_num_cpus) {
            {
                if (m_uart) {
                    int irq = gs::cci_get<int>(std::string(m_uart->name()) + ".irq");
                    m_uart->irq.bind(m_gic->spi_in[irq]);
                }
            }
            for (auto& qup : m_uart_qups) {
                int irq = gs::cci_get<int>(std::string(qup.name()) + ".irq");
                qup.irq.bind(m_gic->spi_in[irq]);
            }
            {
                int irq = gs::cci_get<int>(std::string(m_virtio_net_0.name()) + ".irq");
                m_virtio_net_0.irq_out.bind(m_gic->spi_in[irq]);
            }
#ifdef newsmmu
            for (int i = 0; i < m_smmu_connd_remotes.size(); i++) {
                m_smmu_connd_remotes[i].bind_irqs(m_gic);
            }
#endif
            for (auto& vblk : m_virtio_blks) {
                int irq = gs::cci_get<int>(std::string(vblk.name()) + ".irq");
                vblk.irq_out.bind(m_gic->spi_in[irq]);
            }
            for (auto& qt : m_qtimers) {
                uint32_t nr_frames = gs::cci_get<int>(std::string(qt.name()) + ".nr_frames");
                for (uint32_t i = 0; i < nr_frames; ++i) {
                    int irq = gs::cci_get<int>(std::string(qt.name()) + ".irq" + std::to_string(i));
                    qt.irq[i].bind(m_gic->spi_in[irq]);
                }
            }

            int irq = gs::cci_get<int>(std::string(m_gpex->name()) + ".irq_0");
            m_gpex->irq_out[0].bind(m_gic->spi_in[irq]);
            m_gpex->irq_num[0] = irq;

            irq = gs::cci_get<int>(std::string(m_gpex->name()) + ".irq_1");
            m_gpex->irq_out[1].bind(m_gic->spi_in[irq]);
            m_gpex->irq_num[1] = irq;

            irq = gs::cci_get<int>(std::string(m_gpex->name()) + ".irq_2");
            m_gpex->irq_out[2].bind(m_gic->spi_in[irq]);
            m_gpex->irq_num[2] = irq;

            irq = gs::cci_get<int>(std::string(m_gpex->name()) + ".irq_3");
            m_gpex->irq_out[3].bind(m_gic->spi_in[irq]);
            m_gpex->irq_num[3] = irq;

            for (int i = 0; i < m_cpus.size(); i++) {
                m_gic->irq_out[i].bind(m_cpus[i].irq_in);
                m_gic->fiq_out[i].bind(m_cpus[i].fiq_in);

                m_gic->virq_out[i].bind(m_cpus[i].virq_in);
                m_gic->vfiq_out[i].bind(m_cpus[i].vfiq_in);

                m_cpus[i].irq_timer_phys_out.bind(m_gic->ppi_in[i][ARCH_TIMER_NS_EL1_IRQ]);
                m_cpus[i].irq_timer_virt_out.bind(m_gic->ppi_in[i][ARCH_TIMER_VIRT_IRQ]);
                m_cpus[i].irq_timer_hyp_out.bind(m_gic->ppi_in[i][ARCH_TIMER_NS_EL2_IRQ]);
                m_cpus[i].irq_timer_sec_out.bind(m_gic->ppi_in[i][ARCH_TIMER_S_EL1_IRQ]);
                m_cpus[i].irq_maintenance_out.bind(m_gic->ppi_in[i][25]);
                m_cpus[i].irq_pmu_out.bind(m_gic->ppi_in[i][23]);
            }

            for (auto irqnum :
                 gs::sc_cci_children((std::string(m_ipcc.name()) + ".irqs").c_str())) {
                std::string irqss = (std::string(m_ipcc.name()) + ".irqs." + irqnum);
                int irq = gs::cci_get<int>(irqss + ".irq");
                std::string dstname = gs::cci_get<std::string>(irqss + ".dst");
                sc_core::sc_object* dst_obj = gs::find_sc_obj(nullptr, dstname);
                auto dst = dynamic_cast<QemuTargetSignalSocket*>(dst_obj);
                if (!dst) {
                    auto dst_target_signal_socket = dynamic_cast<TargetSignalSocket<bool>*>(
                        dst_obj);
                    if (!dst_target_signal_socket) {
                        SCP_FATAL(()) << "ipcc.irqs.dst object is not QemuTargetSignalSocket nor "
                                         "TargetSignalSocket";
                    }
                    m_ipcc.irq[irq].bind((*dst_target_signal_socket));
                    continue;
                }
                m_ipcc.irq[irq].bind((*dst));
                //                std::cout << "BINDING IRQ : "<< dstname<<" to
                //                ipcc["<<irq<<"]\n";
            }
        }
    }

public:
    GreenSocsPlatform(const sc_core::sc_module_name& n)
        : sc_core::sc_module(n)
        , p_hexagon_num_clusters("hexagon_num_clusters", 2, "Number of Hexagon clusters")
        , p_arm_num_cpus("arm_num_cpus", 8, "Number of ARM cores")
        , p_num_redists("num_redists", 1, "Number of redistribution regions")
        , p_with_gpu("with_gpu", false, "Build platform with GPU")
        , m_broker({
              { "gic.num_spi", cci::cci_value(960) }, // 64 seems reasonable, but can be up to
                                                      // 960 or 987 depending on how the gic
                                                      // is used MUST be a multiple of 32
              { "gic.redist_region", cci::cci_value(std::vector<unsigned int>(
                                         p_num_redists, p_arm_num_cpus / p_num_redists)) },
          })
        , p_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
        , p_uart_backend("uart_backend_port", 0,
                         "uart backend port number, either 0 for 'stdio' or a "
                         "port number (e.g. 4001)")
        , m_router("router")

        , m_qemu_inst(m_inst_mgr.new_instance("ArmQemuInstance", QemuInstance::Target::AARCH64))
        , m_cpus("cpu", p_arm_num_cpus,
                 [this](const char* n, size_t i) {
                     /* here n is already "cpu_<vector-index>" */
                     return new QemuCpuArmCortexA76(n, m_qemu_inst);
                 })
        , m_hexagon_clusters("hexagon_cluster", p_hexagon_num_clusters,
                             [this](const char* n, size_t i) {
                                 return new hexagon_cluster(n, m_inst_mgr_h, m_router);
                             })
        , m_rams("ram", gs::sc_cci_list_items(sc_module::name(), "ram").size(),
                 [this](const char* n, size_t i) { return new gs::Memory<>(n); })

        , m_hexagon_rams("hexagon_ram",
                         gs::sc_cci_list_items(sc_module::name(), "hexagon_ram").size(),
                         [this](const char* n, size_t i) { return new gs::Memory<>(n); })

        //        , m_system_imem("system_imem")
        , m_uart_qups("uart_qup", gs::sc_cci_list_items(sc_module::name(), "uart_qup").size(),
                      [this](const char* n, size_t i) { return new QUPv3(n); })
        , m_ipcc("ipcc")
        , m_mpm("mpm")
#ifdef newsmmu
        , m_smmus("smmu", gs::sc_cci_list_items(sc_module::name(), "smmu").size(),
                  [this](const char* n, size_t i) { return new smmu500<>(n); })
        , m_tbus("tbu", p_hexagon_num_clusters,
                 [this](const char* n, size_t i) { return new smmu500_tbu<>(n, &m_smmus[0]); })
        , m_smmu_connd_remotes(
              "remote", gs::sc_cci_list_items(sc_module::name(), "remote").size(),
              [this](const char* n, size_t i) { return new SMMUConnectedPassRPC(n, &m_smmus[0]); })
#endif
        , m_virtio_net_0("virtionet0", m_qemu_inst)
        , m_virtio_blks(
              "virtioblk", gs::sc_cci_list_items(sc_module::name(), "virtioblk").size(),
              [this](const char* n, size_t i) { return new QemuVirtioMMIOBlk(n, m_qemu_inst); })
        , m_gpus("gpu", gs::sc_cci_list_items(sc_module::name(), "gpu").size(),
                 [this](const char* n, size_t i) {
                     QemuVirtioGpu* ret = nullptr;
                     if (p_with_gpu.get_value()) {
                         if (i == 0) {
                             // First GPU is accelerated
                             ret = new QemuVirtioGpuGlPci(n, m_qemu_inst);
                         } else {
                             ret = new QemuVirtioGpuPci(n, m_qemu_inst);
                         }
                     }
                     return ret;
                 })
        , m_qtimers(
              "qtimer", gs::sc_cci_list_items(sc_module::name(), "qtimer").size(),
              [this](const char* n, size_t i) { return new QemuHexagonQtimer(n, m_qemu_inst); })
        , m_fallback_mem("fallback_memory")
        , m_memorydumper("memorydumper")
        , m_loader("load") {
        using tlm_utils::tlm_quantumkeeper;

        SCP_DEBUG(()) << "Constructor";

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);

        if (p_arm_num_cpus) {
            m_gic = new QemuArmGicv3("gic", m_qemu_inst, p_arm_num_cpus);
            m_global_peripheral_initiator_arm = new GlobalPeripheralInitiator(
                "glob-per-init-arm", m_qemu_inst, m_cpus[0]);
        }

        uint64_t mmio_addr = gs::cci_get<uint64_t>(std::string(this->name()) +
                                                   ".gpex.mmio_iface.address");
        uint64_t mmio_size = gs::cci_get<uint64_t>(std::string(this->name()) +
                                                   ".gpex.mmio_iface.size");
        uint64_t mmio_iface_high_addr = gs::cci_get<uint64_t>(std::string(this->name()) +
                                                              ".gpex.mmio_iface_high.address");
        uint64_t mmio_iface_high_size = gs::cci_get<uint64_t>(std::string(this->name()) +
                                                              ".gpex.mmio_iface_high.size");

        m_gpex = new QemuGPEX("gpex", m_qemu_inst, mmio_addr, mmio_size, mmio_iface_high_addr,
                              mmio_iface_high_size);

        m_xhci = std::make_unique<QemuXhci>("usb", m_qemu_inst);
        m_gpex->add_device(*m_xhci);
        m_kbd = std::make_unique<QemuKbd>("kbd", m_qemu_inst);
        m_xhci->add_device(*m_kbd);
        m_tablet = std::make_unique<QemuTablet>("mouse", m_qemu_inst);
        m_xhci->add_device(*m_tablet);

        if (p_with_gpu.get_value()) {
            if (m_gpus.size() == 0) {
                SCP_ERR(SCMOD) << "Please specify at least one gpu (gpu_0)";
            }

            for (QemuVirtioGpu& gpu : m_gpus) {
                m_gpex->add_device(gpu);
            }
#ifdef __APPLE__
            if (m_gpus.size() > 0) {
                // Our custom QemuDisplay is only required on MacOS
                m_display = new QemuDisplay("display", m_gpus[0]);
            }
#endif
        } else if (m_gpus.size() > 0) {
            // In case at least one GPU is specified and with_gpu is false
            SCP_WARN(SCMOD)
                << "GPUs are disabled, you can enable them with `platform.with_gpu = true`";
        }

        m_rtl8139 = new QemuRtl8139Pci("rtl8139", m_qemu_inst, m_gpex);
        if (m_rams.size() <= 0) {
            SCP_ERR(SCMOD) << "Please specify at least one memory (ram_0)";
        }

        if (m_broker.has_preset_value(std::string(this->name()) +
                                      ".uart.simple_target_socket_0.address")) {
            uint64_t uart_addr = gs::cci_get<uint64_t>(std::string(this->name()) +
                                                       ".uart.simple_target_socket_0.address");

            if (uart_addr) {
                m_uart = new Uart("uart");
            }
        }

        do_bus_binding();

#ifdef newsmmu
        // Always bind the SMMU even if there are no hexagons, to ensure
        // complete binding
        for (auto& smmu : m_smmus) {
            smmu.dma_socket.bind(m_router.target_socket);
        }
#endif

        if (p_hexagon_num_clusters) {
            for (int N = 0; N < p_hexagon_num_clusters; N++) {
#ifndef newsmmu
                abort(); // fixme multiple SMMU support

                m_smmu = new QemuArmSmmu("smmu", m_hexagon_clusters[N].m_qemu_hex_inst);
                m_hexagon_clusters[N].m_router.initiator_socket.bind(m_smmu->upstream_socket[N]);
                m_smmu->downstream_socket[N].bind(m_router.target_socket);
                m_router.initiator_socket.bind(m_smmu->register_socket);
#else
                m_hexagon_clusters[N].m_router.initiator_socket.bind(m_tbus[N].upstream_socket);
                m_tbus[N].downstream_socket.bind(m_router.target_socket);
#endif
            }
#ifdef newsmmu
            int smmu_index = 0;
            for (auto& smmu : m_smmus) {
                /* FIXME: the ARM GIC model cannot support all of the
                ** interrupts required for all SMMUs, let's just bypass
                ** this for now.
                */
                if (smmu_index > 0) {
                    break;
                }

                int irq = gs::cci_get<int>(std::string(smmu.name()) + ".irq_context");
                int girq = gs::cci_get<int>(std::string(smmu.name()) + ".irq_global");
                for (int i = 0; i < smmu.p_num_cb; i++) {
                    smmu.irq_context[i].bind(m_gic->spi_in[irq + i]);
                }
                smmu.irq_global.bind(m_gic->spi_in[girq]);
                smmu_index++;
            }
#else
            abort(); // FIXME
#endif
        }

#ifdef newsmmu
        for (int i = 0; i < m_smmu_connd_remotes.size(); i++) {
            m_smmu_connd_remotes[i].bind_tlm_ports(m_router);
        }
#endif

        // General loader
        m_loader.initiator_socket.bind(m_router.target_socket);

        m_router.initiator_socket.bind(m_memorydumper.target_socket);
        m_memorydumper.initiator_socket.bind(m_router.target_socket);

        // MUST be added last
        m_router.initiator_socket.bind(m_fallback_mem.socket);

        setup_irq_mapping();

        bool used_stdin = false;
        int stdio_index = 0;
        for (auto& qup : m_uart_qups) {
            bool be = gs::cci_get<bool>(std::string(qup.name()) + ".stdio");
            if (be) {
                const std::string input(std::string(qup.name()) + ".input");
                const bool excl_stdin = m_broker.has_preset_value(input) &&
                                        gs::cci_get<bool>(input);
                if (used_stdin && excl_stdin) {
                    SCP_FATAL(())("Only one Uart may claim INPUT at any one time\n");
                }
                used_stdin = used_stdin || excl_stdin;

                const std::string stdio_name("backend_stdio_" + std::to_string(stdio_index++));
                qup.set_backend(new CharBackendStdio(stdio_name.c_str(), excl_stdin));
            } else {
                bool has_port = m_broker.has_preset_value(std::string(qup.name()) + ".port");
                if (!has_port) {
                    SCP_WARN(())("Uart '{}' not connected", qup.name());
                } else {
                    int port = gs::cci_get<int>(std::string(qup.name()) + ".port");
                    qup.set_backend(new CharBackendSocket("qup_backend_socket", "tcp",
                                                          "127.0.0.1:" + std::to_string(port)));
                }
            }
        }

        if (m_uart) {
            bool be = gs::cci_get<bool>(std::string(m_uart->name()) + ".stdio");
            if (be) {
                const std::string input(std::string(m_uart->name()) + ".input");
                const bool excl_stdin = m_broker.has_preset_value(input) &&
                                        gs::cci_get<bool>(input);
                if (used_stdin && excl_stdin) {
                    SCP_FATAL(())("Only one Uart may claim INPUT at any one time\n");
                }
                used_stdin = used_stdin || excl_stdin;
                m_uart->set_backend(new CharBackendStdio("uart_backend_stdio", excl_stdin));
            } else {
                bool has_port = m_broker.has_preset_value(std::string(m_uart->name()) + ".port");
                if (!has_port) {
                    SCP_WARN(())("Uart '{}' not connected", m_uart->name());
                } else {
                    int port = gs::cci_get<int>(std::string(m_uart->name()) + ".port");
                    m_uart->set_backend(new CharBackendSocket("qup_backend_socket", "tcp",
                                                              "127.0.0.1:" + std::to_string(port)));
                }
            }
        }
        if (!used_stdin) {
            SCP_WARN(())("No UART configured to use STDIO");
        }
    }

    SCP_LOGGER(());

    ~GreenSocsPlatform() {
        if (m_global_peripheral_initiator_arm) {
            delete m_global_peripheral_initiator_arm;
        }
        if (m_gic) {
            delete m_gic;
        }
#ifndef newsmmu
        if (m_smmu) {
            delete m_smmu;
        }
#endif
        if (m_display) {
            delete m_display;
        }
        delete m_gpex;
        delete m_rtl8139;

        if (m_uart) {
            delete m_uart;
        }
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
    gs::ConfigurableBroker m_broker(argc, argv, true);
    cci::cci_originator orig("sc_main");
    cci::cci_param<int> p_log_level{ "log_level", 0, "Default log level", cci::CCI_ABSOLUTE_NAME,
                                     orig };
    GreenSocsPlatform* platform = new GreenSocsPlatform("platform");

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

    free(platform);
    return 0;
}
