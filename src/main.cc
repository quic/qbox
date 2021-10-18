/*
 *  Copyright (C) 2021 GreenSocs SAS
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

#include <systemc>
#include <cci_configuration>

#include <greensocs/gsutils/luafile_tool.h>
#include <greensocs/gsutils/cciutils.h>

#include <libqbox/components/cpu/arm/cortex-a53.h>
#include <libqbox/components/cpu/hexagon/hexagon.h>
#include <libqbox/components/irq-ctrl/arm-gicv3.h>
#include <libqbox/components/uart/pl011.h>

#include <greensocs/base-components/router.h>
#include <greensocs/base-components/memory.h>

#define KERNEL64_LOAD_ADDR 0x00080000
#define DTB_LOAD_ADDR      0x08000000

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

static const uint32_t bootloader_aarch64[] = {
    /* 0000000000000000 <start>: */
    0xd53800a6, /*	mrs	x6, mpidr_el1 */
    0x580001c7, /*	ldr	x7, 3c <aff_mask> */
    0x8a0600e7, /*	and	x7, x7, x6 */
    0xf10000ff, /*	cmp	x7, #0x0 */
    0x54000061, /*	b.ne	1c <secondary>  // b.any */
    /* 0000000000000014 <core0>: */
    0x580001a4, /*	ldr	x4, 48 <kernel_entry> */
    0x14000004, /*	b	28 <boot> */
    /* 000000000000001c <secondary>: */
    0xd503205f, /*	wfe */
    0x58000304, /*	ldr	x4, 80 <spintable> */
    0xb4ffffc4, /*	cbz	x4, 1c <secondary> */
    /* 0000000000000028 <boot>: */
    0x580000c0, /*	ldr	x0, 40 <dtb_ptr> */
    0xaa1f03e1, /*	mov	x1, xzr */
    0xaa1f03e2, /*	mov	x2, xzr */
    0xaa1f03e3, /*	mov	x3, xzr */
    0xd61f0080, /*	br	x4 */
    /* 000000000000003c <aff_mask>: */
    0x00ffffff, /*	.word	0x00ffffff */
    /* 0000000000000040 <dtb_ptr>: */
    DTB_LOAD_ADDR, /*	.word	0x08000000 */
    0x00000000,    /*	.word	0x00000000 */
    /* 0000000000000048 <kernel_entry>: */
    KERNEL64_LOAD_ADDR, /*	.word	0x00080000 */
    0x00000000,         /*	.word	0x00000000 */
    /* 0000000000000050 <reserved>: */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
    /* 0000000000000080 <spintable>: */
    0x00000000, /*	.word	0x00000000 */
    0x00000000, /*	.word	0x00000000 */
};

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

class GreenSocsPlatform : public sc_core::sc_module {
protected:
    gs::ConfigurableBroker m_broker;

    cci::cci_param<int> m_quantum_ns;
    cci::cci_param<int> m_gdb_port;
    cci::cci_param<cci::uint64> m_ram_size;

    /* Memory direct loading params */
    cci::cci_param<std::string> m_ram_blob_file;
    cci::cci_param<std::string> m_flash_blob_file;

    /* OpenSBI/Linux bootloader params */
    cci::cci_param<std::string> m_kernel_file;
    cci::cci_param<std::string> m_dtb_file;

    /* Hexagon bootloader params */
    cci::cci_param<std::string> m_hexagon_kernel_file;
    cci::cci_param<cci::uint64> m_hexagon_load_addr;
    cci::cci_param<uint32_t> m_hexagon_start_addr;

    /* Address map params */
    cci::cci_param<cci::uint64> m_addr_map_ram;
    cci::cci_param<cci::uint64> m_addr_map_uart;

    QemuInstanceManager m_inst_mgr;
    QemuInstance &m_qemu_inst;
    QemuInstance &m_qemu_hex_inst;

    sc_core::sc_vector<QemuCpuArmCortexA53> m_cpus;
    QemuCpuHexagon m_hexagon;
    QemuArmGicv3 m_gic;
    Router<> m_router;
    Memory m_ram;
    Memory m_flash;
    QemuUartPl011 m_uart;

    hexagon_config_table *cfgTable;
    hexagon_config_extensions *cfgExtensions;
    void setup_cpus() {
        for (int i = 0; i < m_cpus.size(); i++) {
            auto &cpu = m_cpus[i];

            cpu.p_has_el3 = false;
            cpu.p_psci_conduit = "hvc";
            cpu.p_mp_affinity = ((i / 4) << 8) | (i % 4);

            if (i == 0) {
                if (!m_gdb_port.is_default_value()) {
                    cpu.p_gdb_port = m_gdb_port;
                }
            } else {
                cpu.p_start_powered_off = true;
            }
        }
    }

    void setup_memory_mapping() {
        for (auto &cpu: m_cpus) {
            m_router.add_initiator(cpu.socket);
        }
        m_router.add_initiator(m_hexagon.socket);
        m_router.add_target(m_ram.socket, m_addr_map_ram, m_ram.size());
        m_router.add_target(m_flash.socket, 0x200000000, m_flash.size());
        m_router.add_target(m_gic.dist_iface, 0xc8000000, 0x10000);
        m_router.add_target(m_gic.redist_iface[0], 0xc8010000, 0x20000);
        m_router.add_target(m_uart.socket, m_addr_map_uart, 0x1000);

    }

    void setup_irq_mapping() {
        for (int i = 0; i < m_cpus.size(); i++) {
            m_gic.irq_out[i].bind(m_cpus[i].irq_in);
            m_gic.fiq_out[i].bind(m_cpus[i].fiq_in);

            m_cpus[i].irq_timer_phys_out.bind(m_gic.ppi_in[i][ARCH_TIMER_NS_EL1_IRQ]);
            m_cpus[i].irq_timer_virt_out.bind(m_gic.ppi_in[i][ARCH_TIMER_VIRT_IRQ]);
            m_cpus[i].irq_timer_hyp_out.bind(m_gic.ppi_in[i][ARCH_TIMER_NS_EL2_IRQ]);
            m_cpus[i].irq_timer_sec_out.bind(m_gic.ppi_in[i][ARCH_TIMER_S_EL1_IRQ]);
        }

        m_uart.irq_out.bind(m_gic.spi_in[1]);
    }

    bool load_blobs()
    {
        bool success = false;

        if (!m_flash_blob_file.is_default_value()) {
            m_flash.load(m_flash_blob_file, 0);
        }

        if (!m_ram_blob_file.is_default_value()) {
            m_ram.load(m_ram_blob_file, 0);
            success = true;
        }

        return success;
    }

    void do_bootloader()
    {
        if (load_blobs()) {
            return;
        }

        if (!m_kernel_file.is_default_value()) {
            m_ram.load(m_kernel_file, KERNEL64_LOAD_ADDR);
        }

        if (!m_hexagon_kernel_file.is_default_value()) {
            m_ram.load(m_hexagon_kernel_file, m_hexagon_load_addr);
        }

        if (!m_dtb_file.is_default_value()) {
            m_ram.load(m_dtb_file, DTB_LOAD_ADDR);
        }

        m_ram.load(reinterpret_cast<const uint8_t *>(bootloader_aarch64), sizeof(bootloader_aarch64), 0x0);

        hexagon_config_table *config_table = cfgTable;

        config_table->l2tcm_base = HEXAGON_CFG_ADDR_BASE(cfgTable->l2tcm_base);
        config_table->subsystem_base = HEXAGON_CFG_ADDR_BASE(cfgExtensions->csr_base);
        config_table->vtcm_base = HEXAGON_CFG_ADDR_BASE(cfgTable->vtcm_base);
        config_table->l2cfg_base = HEXAGON_CFG_ADDR_BASE(cfgTable->l2cfg_base);
        config_table->fastl2vic_base =
                             HEXAGON_CFG_ADDR_BASE(cfgTable->fastl2vic_base);

       //m_ram.load(reinterpret_cast<const uint8_t *>(hexagon_bootstrap),sizeof(hexagon_bootstrap), m_hexagon_start_addr);

    }

public:
    GreenSocsPlatform(const sc_core::sc_module_name &n)
        : sc_core::sc_module(n)
        , m_broker({
            {"gic.num-cpu", cci::cci_value(4)},
            {"gic.num-spi", cci::cci_value(64)},
            {"gic.redist-region", cci::cci_value(std::vector<unsigned int>({32}))},
        })
        , m_quantum_ns("quantum-ns", 1000000, "TLM-2.0 global quantum in ns")
        , m_gdb_port("gdb_port", 0, "GDB port")
        , m_ram_size("ram_size", 256 * 1024 * 1024, "RAM size")

        , m_ram_blob_file("ram_blob_file", "", "Blob file to load into the RAM")
        , m_flash_blob_file("flash_blob_file", "", "Blob file to load into the flash")

        , m_kernel_file("kernel_file", "", "Kernel blob file")
        , m_dtb_file("dtb_file", "", "Device tree blob to load")

        , m_hexagon_kernel_file("hexagon_kernel_file", "", "Hexagon kernel blob file")
        , m_hexagon_load_addr("hexagon_load_addr",0x100,"Hexagon load address")
        , m_hexagon_start_addr("hexagon_start_addr", 0x100, "Hexagon execution start address")

        , m_addr_map_ram("addr_map_ram", 0x00000000, "")
        , m_addr_map_uart("addr_map_uart", 0xc0000000, "")

        , m_qemu_inst(m_inst_mgr.new_instance(QemuInstance::Target::AARCH64))
        , m_qemu_hex_inst(m_inst_mgr.new_instance(QemuInstance::Target::HEXAGON))
        , m_cpus("cpu", 4, [this] (const char *n, size_t i) { return new QemuCpuArmCortexA53(n, m_qemu_inst); })
        , m_hexagon("hexagon", m_qemu_hex_inst, v68n_1024_extensions.cfgbase, QemuCpuHexagon::v68_rev, m_hexagon_start_addr)
        , m_gic("gic", m_qemu_inst)
        , m_router("router")
        , m_ram("ram", m_ram_size)
        , m_flash("flash", 256 * 1024 * 1024)
        , m_uart("uart", m_qemu_inst)

    {
        using tlm_utils::tlm_quantumkeeper;
        cfgTable = &v68n_1024_cfgtable;
        cfgExtensions = &v68n_1024_extensions;
        sc_core::sc_time global_quantum(m_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);

        setup_cpus();
        setup_memory_mapping();
        setup_irq_mapping();

        do_bootloader();
    }
};

int sc_main(int argc, char *argv[])
{
    auto m_broker = new gs::ConfigurableBroker(argc, argv);

    GreenSocsPlatform platform("platform");

    auto start = std::chrono::system_clock::now();
    sc_core::sc_start();
    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds() << "SC_SEC" << std::endl;
    std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)" << std::endl;

    return 0;
}

