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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
* You may obtain a copy of the Apache License at
* http://www.apache.org/licenses/LICENSE-2.0
*/

#include <chrono>
#include <string>

#include <systemc>
#include <cci_configuration>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <greensocs/gsutils/luafile_tool.h>
#include <greensocs/gsutils/cciutils.h>

#include <libqbox/components/cpu/arm/max.h>
#include <libqbox/components/cpu/hexagon/hexagon.h>
#include <libqbox/components/irq-ctrl/arm-gicv3.h>
#include <libqbox/components/uart/pl011.h>
#include <libqbox/components/irq-ctrl/hexagon-l2vic.h>
#include <libqbox/components/timer/hexagon-qtimer.h>
#include <libqbox/components/net/virtio-mmio-net.h>
#include <libqbox/components/blk/virtio-mmio-blk.h>
#include <libqbox/components/mmu/arm-smmu.h>

#include <libqbox-extra/components/meta/global_peripheral_initiator.h>

#include <greensocs/base-components/router.h>
#include <greensocs/base-components/memory.h>
#include <greensocs/base-components/connectors.h>
#include "greensocs/systemc-uarts/uart-pl011.h"
#include "greensocs/systemc-uarts/backends/char-backend.h"
#include <greensocs/systemc-uarts/backends/char/stdio.h>
#include <greensocs/systemc-uarts/backends/char/socket.h>

#include <qcom/ipcc/ipcc.h>
#include <qcom/qtb/qtb.h>

#define HEXAGON_CFGSPACE_ENTRIES (128)
#define HEXAGON_CFG_ADDR_BASE(addr) ((addr >> 16) & 0x0fffff)


typedef struct {
    uint32_t l2tcm_base; /* Base address of L2TCM space */
    uint32_t reserved; /* Reserved */
    uint32_t subsystem_base; /* Base address of subsystem space */
    uint32_t etm_base; /* Base address of ETM space */
    uint32_t l2cfg_base; /* Base address of L2 configuration space */
    uint32_t reserved2;
    uint32_t l1s0_base; /* Base address of L1S */
    uint32_t axi2_lowaddr; /* Base address of AXI2 */
    uint32_t streamer_base; /* Base address of streamer base */
    uint32_t clade_base; /* Base address of Clade */
    uint32_t fastl2vic_base; /* Base address of fast L2VIC */
    uint32_t jtlb_size_entries; /* Number of entries in JTLB */
    uint32_t coproc_present; /* Coprocessor type */
    uint32_t ext_contexts; /* Number of extension execution contexts available */
    uint32_t vtcm_base; /* Base address of Hexagon Vector Tightly Coupled Memory (VTCM) */
    uint32_t vtcm_size_kb; /* Size of VTCM (in KB) */
    uint32_t l2tag_size; /* L2 tag size */
    uint32_t l2ecomem_size; /* Amount of physical L2 memory in released version */
    uint32_t thread_enable_mask; /* Hardware threads available on the core */
    uint32_t eccreg_base; /* Base address of the ECC registers */
    uint32_t l2line_size; /* L2 line size */
    uint32_t tiny_core; /* Small Core processor (also implies audio extension) */
    uint32_t l2itcm_size; /* Size of L2TCM */
    uint32_t l2itcm_base; /* Base address of L2-ITCM */
    uint32_t clade2_base; /* Base address of Clade2 */
    uint32_t dtm_present; /* DTM is present */
    uint32_t dma_version; /* Version of the DMA */
    uint32_t hvx_vec_log_length; /* Native HVX vector length in log of bytes */
    uint32_t core_id; /* Core ID of the multi-core */
    uint32_t core_count; /* Number of multi-core cores */
    uint32_t hmx_int8_spatial; /* FIXME: undocumented */
    uint32_t hmx_int8_depth; /* FIXME: undocumented */
    uint32_t v2x_mode; /* Supported HVX vector length, see HexagonVecLenSupported */
    uint32_t hmx_int8_rate; /* FIXME: undocumented */
    uint32_t hmx_fp16_spatial; /* FIXME: undocumented */
    uint32_t hmx_fp16_depth; /* FIXME: undocumented */
    uint32_t hmx_fp16_rate; /* FIXME: undocumented */
    uint32_t hmx_fp16_acc_frac; /* FIXME: undocumented */
    uint32_t hmx_fp16_acc_int; /* FIXME: undocumented */
    uint32_t acd_preset; /* Voltage droop mitigation technique parameter */
    uint32_t mnd_preset; /* Voltage droop mitigation technique parameter */
    uint32_t l1d_size_kb; /* L1 data cache size (in KB) */
    uint32_t l1i_size_kb; /* L1 instruction cache size in (KB) */
    uint32_t l1d_write_policy; /* L1 data cache write policy: see HexagonL1WritePolicy */
    uint32_t vtcm_bank_width; /* VTCM bank width  */
    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t reserved5;
    uint32_t hmx_cvt_mpy_size; /* FIXME: undocumented Size of the fractional multiplier in the HMX Covert */
    uint32_t axi3_lowaddr; /* FIXME: undocumented */
} hexagon_config_table;

typedef struct {
    uint32_t cfgbase; /* Base address of config table */
    uint32_t cfgtable_size; /* Size of config table */
    uint32_t l2tcm_size; /* Size of L2 TCM */
    uint32_t l2vic_base; /* Base address of L2VIC */
    uint32_t l2vic_size; /* Size of L2VIC region */
    uint32_t csr_base; /* QTimer csr base */
    uint32_t qtmr_rg0;
    uint32_t qtmr_rg1;
} hexagon_config_extensions;


hexagon_config_table v68n_1024_cfgtable = {
 /* The following values were derived from hexagon-sim, QuRT config files, */
 /* and the hexagon scoreboard for v68n_1024 found here: http://go/q6area  */
    .l2tcm_base =           0xd8000000,
    .reserved =             0x00000000,
    .subsystem_base =       0x00002000,
    .etm_base =             0xd8190000,
    .l2cfg_base =           0xd81a0000,
    .reserved2 =            0x00000000,
    .l1s0_base =            0xd8400000,
    .axi2_lowaddr =         0x00003000,
    .streamer_base =        0xd81c0000,
    .clade_base =           0xd81d0000,
    .fastl2vic_base =       0xd81e0000,
    .jtlb_size_entries =    0x00000080,
    .coproc_present =       0x00000001,
    .ext_contexts =         0x00000004,
    .vtcm_base =            0xd8400000,
    .vtcm_size_kb =         0x00001000,
    .l2tag_size =           0x00000400,
    .l2ecomem_size =        0x00000400,
    .thread_enable_mask =   0x0000003f,
    .eccreg_base =          0xd81f0000,
    .l2line_size =          0x00000080,
    .tiny_core =            0x00000000,
    .l2itcm_size =          0x00000000,
    .l2itcm_base =          0xd8200000,
    .clade2_base =          0x00000000,
    .dtm_present =          0x00000000,
    .dma_version =          0x00000001,
    .hvx_vec_log_length =   0x00000007,
    .core_id =              0x00000000,
    .core_count =           0x00000000,
    .hmx_int8_spatial =     0x00000040,
    .hmx_int8_depth =       0x00000020,
    .v2x_mode =             0x1f1f1f1f,
    .hmx_int8_rate =        0x1f1f1f1f,
    .hmx_fp16_spatial =     0x1f1f1f1f,
    .hmx_fp16_depth =       0x1f1f1f1f,
    .hmx_fp16_rate =        0x1f1f1f1f,
    .hmx_fp16_acc_frac =    0x1f1f1f1f,
    .hmx_fp16_acc_int =     0x1f1f1f1f,
    .acd_preset =           0x1f1f1f1f,
    .mnd_preset =           0x1f1f1f1f,
    .l1d_size_kb =          0x1f1f1f1f,
    .l1i_size_kb =          0x1f1f1f1f,
    .l1d_write_policy =     0x1f1f1f1f,
    .vtcm_bank_width =      0x1f1f1f1f,
    .reserved3 =            0x1f1f1f1f,
    .reserved4 =            0x1f1f1f1f,
    .reserved5 =            0x1f1f1f1f,
    .hmx_cvt_mpy_size =     0x1f1f1f1f,
    .axi3_lowaddr =         0x1f1f1f1f,
};

hexagon_config_extensions v68n_1024_extensions = {
    .cfgbase =           0xde000000,
    .cfgtable_size =     0x00000400,
    .l2tcm_size =        0x00000000,
    .l2vic_base =        0xfc910000,
    .l2vic_size =        0x00001000,
    .csr_base =          0xfc900000,
    .qtmr_rg0 =          0xfc921000,
    .qtmr_rg1 =          0xfc922000,
};
/*
uint32_t hexagon_bootstrap[] = {

0x07004000, // { immext(#1879048192)
0x7800c000, // r0 = ##1879048192 }
0x7800c001, // { r1 = #0 }
//0000000c <write_loop>:
0xa180c100, // { memw(r0+#0) = r1 }
0xb001c021, // { r1 = add(r1,#1) }
0x5440cc10, // { pause(#100) }
0x59fffffa //{ jump 0xc <write_loop> }

};
*/
#define ARCH_TIMER_VIRT_IRQ   (16+11)
#define ARCH_TIMER_S_EL1_IRQ  (16+13)
#define ARCH_TIMER_NS_EL1_IRQ (16+14)
#define ARCH_TIMER_NS_EL2_IRQ (16+10)

class hexagon_cluster : public sc_core::sc_module {
public:
    cci::cci_param<unsigned> p_hexagon_num_threads;
    cci::cci_param<uint32_t> p_hexagon_start_addr;

        // keep public so that smmu can be associated with hexagon
    QemuInstance &m_qemu_hex_inst;
private:
    QemuHexagonL2vic m_l2vic;
    QemuHexagonQtimer m_qtimer;
    gs::pass<> m_pass;

    sc_core::sc_vector<QemuCpuHexagon> m_hexagon_threads;
    GlobalPeripheralInitiator m_global_peripheral_initiator_hex;
public:
    // expose the router, to facilitate binding
    // otherwise we would need a 'bridge' to bind to the router and an initiator socket.
    gs::Router<> m_router;

    hexagon_cluster(const sc_core::sc_module_name& n, QemuInstanceManager &m_inst_mgr, gs::Router<> &parent_router)
        : sc_core::sc_module(n)
        , p_hexagon_num_threads("hexagon_num_threads", 8, "Number of Hexagon threads")
        , p_hexagon_start_addr("hexagon_start_addr", 0x100, "Hexagon execution start address")

        , m_qemu_hex_inst(m_inst_mgr.new_instance("HexagonQemuInstance", QemuInstance::Target::HEXAGON))
        , m_l2vic("l2vic", m_qemu_hex_inst)
        , m_qtimer("qtimer", m_qemu_hex_inst) // are we sure it's in the hex cluster?????
        , m_pass("pass", false)
        , m_hexagon_threads("hexagon_thread", p_hexagon_num_threads, [this] (const char *n, size_t i) {
            /* here n is already "hexagon-cpu_<vector-index>" */
            return new QemuCpuHexagon(n, m_qemu_hex_inst,
                                      v68n_1024_extensions.cfgbase,
                                      QemuCpuHexagon::v68_rev,
                                      v68n_1024_extensions.l2vic_base,
                                      v68n_1024_extensions.qtmr_rg0,
                                      p_hexagon_start_addr);
        })
        , m_router("router")
        , m_global_peripheral_initiator_hex("glob-per-init-hex", m_qemu_hex_inst, m_hexagon_threads[0])
    {
        parent_router.initiator_socket.bind(m_l2vic.socket);
        parent_router.initiator_socket.bind(m_l2vic.socket_fast);
        parent_router.initiator_socket.bind(m_qtimer.socket);
        parent_router.initiator_socket.bind(m_qtimer.timer0_socket);
        parent_router.initiator_socket.bind(m_qtimer.timer1_socket);

        // pass through transactions.
        m_router.initiator_socket.bind(m_pass.target_socket);
        m_pass.initiator_socket.bind(parent_router.target_socket);

        for (auto& cpu : m_hexagon_threads) {
            cpu.socket.bind(m_router.target_socket);
        }

        for(int i = 0; i < m_l2vic.p_num_outputs; ++i) {
            m_l2vic.irq_out[i].bind(m_hexagon_threads[0].irq_in[i]);
        }

        m_qtimer.timer0_irq.bind(m_l2vic.irq_in[3]); // FIXME: Depends on static boolean syscfg_is_linux, may be 2
        m_qtimer.timer1_irq.bind(m_l2vic.irq_in[4]);

        m_global_peripheral_initiator_hex.m_initiator.bind(m_router.target_socket);

    }
};

class GreenSocsPlatform : public sc_core::sc_module {
protected:

    cci::cci_param<unsigned> p_arm_num_cpus;
    cci::cci_param<unsigned> p_num_redists;
    cci::cci_param<unsigned> p_hexagon_num_clusters;

    gs::ConfigurableBroker m_broker;

    cci::cci_param<int> p_quantum_ns;
    cci::cci_param<int> p_uart_backend;

    QemuInstanceManager m_inst_mgr;
    QemuInstanceManager m_inst_mgr_h;
    QemuInstance &m_qemu_inst;

    gs::Router<> m_router;
    sc_core::sc_vector<QemuCpuArmMax> m_cpus;
    sc_core::sc_vector<hexagon_cluster> m_hexagon_clusters;
    QemuArmGicv3* m_gic;
    gs::Memory<> m_ram;
    gs::Memory<> m_hexagon_ram;
    gs::Memory<> m_rom;
//    gs::Memory<> m_system_imem;
    GlobalPeripheralInitiator* m_global_peripheral_initiator_arm;
    Uart m_uart;
    IPCC m_ipcc;

    QemuVirtioMMIONet m_virtio_net_0;
    QemuVirtioMMIOBlk m_virtio_blk_0;

    gs::Memory<> m_fallback_mem;

    gs::Loader<> m_loader;

    QemuArmSmmu *m_smmu=nullptr;
    qtb<> *m_qtb;

    hexagon_config_table *cfgTable;
    hexagon_config_extensions *cfgExtensions;

    void do_bus_binding() {
        if (p_arm_num_cpus) {
            for (auto &cpu: m_cpus) {
                cpu.socket.bind(m_router.target_socket);
            }

            m_router.initiator_socket.bind(m_gic->dist_iface);
            m_router.initiator_socket.bind(m_gic->redist_iface[0]);
            m_global_peripheral_initiator_arm->m_initiator.bind(m_router.target_socket);
        }

        m_router.initiator_socket.bind(m_ram.socket);
        m_router.initiator_socket.bind(m_hexagon_ram.socket);
        m_router.initiator_socket.bind(m_rom.socket);
        m_router.initiator_socket.bind(m_uart.socket);
        m_router.initiator_socket.bind(m_ipcc.socket);
        m_router.initiator_socket.bind(m_virtio_net_0.socket);
        m_router.initiator_socket.bind(m_virtio_blk_0.socket);

//        m_router.initiator_socket.bind(m_system_imem.socket);

        // General loader
        m_loader.initiator_socket.bind(m_router.target_socket);

        // MUST be added last
        m_router.initiator_socket.bind(m_fallback_mem.socket);
    }

     void setup_irq_mapping() {

        if (p_arm_num_cpus) {
            {
                int irq=gs::cci_get<int>(std::string(m_uart.name())+".irq");
                m_uart.irq.bind(m_gic->spi_in[irq]);
            }
            {
                int irq=gs::cci_get<int>(std::string(m_virtio_net_0.name())+".irq");
                m_virtio_net_0.irq_out.bind(m_gic->spi_in[irq]);
                irq=gs::cci_get<int>(std::string(m_virtio_blk_0.name())+".irq");
                m_virtio_blk_0.irq_out.bind(m_gic->spi_in[irq]);
            }

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

            for (auto irqnum : gs::sc_cci_children((std::string(m_ipcc.name())+".irqs").c_str())) {
                std::string irqss=(std::string(m_ipcc.name())+".irqs."+irqnum);
                int irq=gs::cci_get<int>(irqss+".irq");
                std::string dstname=gs::cci_get<std::string>(irqss+".dst");
                sc_core::sc_object* dst_obj=gs::find_sc_obj(nullptr, dstname);
                auto dst = dynamic_cast<QemuTargetSignalSocket*>(dst_obj);
                m_ipcc.irq[irq].bind((*dst));
//                std::cout << "BINDING IRQ : "<< dstname<<" to ipcc["<<irq<<"]\n";
            }
        }
     }

    void hexagon_config_setup()
    {
        cfgTable = &v68n_1024_cfgtable;
        cfgExtensions = &v68n_1024_extensions;
        std::string n=name();
        cfgTable->fastl2vic_base = gs::cci_get<uint64_t>(n+".cfgTable.fastl2vic_base");
        cfgExtensions->cfgtable_size = gs::cci_get<uint64_t>(n+".cfgExtensions.cfgtable_size");
        cfgExtensions->l2vic_base = gs::cci_get<uint64_t>(n+".cfgExtensions.l2vic_base");
        cfgExtensions->qtmr_rg0 = gs::cci_get<uint64_t>(n+".cfgExtensions.qtmr_rg0");
        cfgExtensions->qtmr_rg1 = gs::cci_get<uint64_t>(n+".cfgExtensions.qtmr_rg1");
    }

public:
    GreenSocsPlatform(const sc_core::sc_module_name &n)
        : sc_core::sc_module(n)
        , p_hexagon_num_clusters("hexagon_num_clusters", 2, "Number of Hexagon cluster")
        , p_arm_num_cpus("arm_num_cpus", 8, "Number of ARM cores")
        , p_num_redists("num_redists", 1, "Number of redistribution regions")
        , m_broker({
            {"gic.num_spi", cci::cci_value(960)}, // 64 seems reasonable, but can be up to 960 or 987 depending on how the gic is used
                                                  // MUST be a multiple of 32
            {"gic.redist_region", cci::cci_value(std::vector<unsigned int>(p_num_redists, p_arm_num_cpus/p_num_redists)) },
        })
        , p_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
        , p_uart_backend("uart_backend_port", 0, "uart backend port number, either 0 for 'stdio' or a port number (e.g. 4001)")

        , m_router("router")

        , m_qemu_inst(m_inst_mgr.new_instance("ArmQemuInstance", QemuInstance::Target::AARCH64))
        , m_cpus("cpu", p_arm_num_cpus, [this] (const char *n, size_t i) {
            /* here n is already "cpu_<vector-index>" */
            return new QemuCpuArmMax(n, m_qemu_inst);
        })
        , m_hexagon_clusters("hexagon_cluster", p_hexagon_num_clusters, [this] (const char *n, size_t i) {
            return new hexagon_cluster(n, m_inst_mgr_h, m_router);
        })
        , m_ram("ram")
        , m_hexagon_ram("hexagon_ram")
        , m_rom("rom")
//        , m_system_imem("system_imem")
        , m_uart("uart")
        , m_ipcc("ipcc")
        , m_virtio_net_0("virtionet0", m_qemu_inst)
        , m_virtio_blk_0("virtioblk0", m_qemu_inst)
        , m_fallback_mem("fallback_memory")
        , m_loader("load")
    {
        using tlm_utils::tlm_quantumkeeper;

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);

        if (p_arm_num_cpus) {
            m_gic = new QemuArmGicv3("gic", m_qemu_inst, p_arm_num_cpus);
            m_global_peripheral_initiator_arm = new GlobalPeripheralInitiator("glob-per-init-arm", m_qemu_inst, m_cpus[0]);
        }

        if (p_hexagon_num_clusters) {
            hexagon_config_setup();
            if (p_hexagon_num_clusters==1) {
                m_smmu = new QemuArmSmmu("smmu", m_hexagon_clusters[0].m_qemu_hex_inst);
                m_hexagon_clusters[0].m_router.initiator_socket.bind(m_smmu->upstream_socket[0]);
                m_smmu->downstream_socket[0].bind(m_router.target_socket);
                m_router.initiator_socket.bind(m_smmu->register_socket);

                m_qtb= new qtb<>("qtb");
                m_router.initiator_socket(m_qtb->control_socket);
                m_qtb->initiator_socket(m_smmu->upstream_socket[1]);
                m_smmu->downstream_socket[1](m_qtb->target_socket);

                m_smmu->dma_socket.bind(m_router.target_socket);

                {
                    int irq=gs::cci_get<int>(std::string(m_smmu->name())+".irq_context");
                    int girq=gs::cci_get<int>(std::string(m_smmu->name())+".irq_global");
                    for (int i=0; i<m_smmu->p_num_cb;i++) {
                        m_smmu->irq_context[i].bind(m_gic->spi_in[irq+i]);
                    }
                    m_smmu->irq_global.bind(m_gic->spi_in[girq]);
                }
            } else {
                for (auto &cpu: m_hexagon_clusters) {
                    cpu.m_router.initiator_socket.bind(m_router.target_socket);
                }
            }
        }

        do_bus_binding();
        setup_irq_mapping();

        CharBackend *backend;
        if (!p_uart_backend) {
            backend=new CharBackendStdio("backend_stdio");
        } else {
            backend=new CharBackendSocket("backend_socket", "tcp", "127.0.0.1:"+std::to_string(p_uart_backend.get_value()));
        }

        m_uart.set_backend(backend);
    }

    ~GreenSocsPlatform() {
        if (m_global_peripheral_initiator_arm) {
            delete m_global_peripheral_initiator_arm;
        }
        if (m_gic) {
            delete m_gic;
        }
        if (m_smmu) {
            delete m_smmu;
        }
    }

    /* this re-writing would seem to be necissary as the values in the configuration want to be different from those read by the device */
    void end_of_elaboration()
    {
        if (!p_hexagon_num_clusters) return;

        hexagon_config_table* config_table = cfgTable;

        config_table->l2tcm_base = HEXAGON_CFG_ADDR_BASE(cfgTable->l2tcm_base);
        config_table->subsystem_base = HEXAGON_CFG_ADDR_BASE(cfgExtensions->csr_base);
        config_table->vtcm_base = HEXAGON_CFG_ADDR_BASE(cfgTable->vtcm_base);
        config_table->l2cfg_base = HEXAGON_CFG_ADDR_BASE(cfgTable->l2cfg_base);
        config_table->fastl2vic_base = HEXAGON_CFG_ADDR_BASE(cfgTable->fastl2vic_base);
        m_loader.ptr_load(reinterpret_cast<uint8_t*>(cfgTable), m_hexagon_ram.base(), sizeof(hexagon_config_table));
    }
};

int sc_main(int argc, char *argv[])
{
    auto m_broker = new gs::ConfigurableBroker(argc, argv);

    GreenSocsPlatform platform("platform");

    auto start = std::chrono::system_clock::now();
    try {
        sc_core::sc_start();
    } catch (std::runtime_error const &e) {
        std::cerr << "Error: '" << e.what() << "'\n";
        exit(1);
    } catch (...) {
        std::cerr << "Unknown error!\n";
        exit(2);
    }

    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds() << "SC_SEC" << std::endl;
    std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)" << std::endl;

    return 0;
}
