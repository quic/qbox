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

#include <libqbox/components/cpu/arm/neoverse-n1.h>
#include <libqbox/components/irq-ctrl/arm-gicv3.h>
#include <libqbox/components/uart/pl011.h>

#include <greensocs/base-components/router.h>
#include <greensocs/base-components/memory.h>

#define KERNEL64_LOAD_ADDR 0x00080000
#define DTB_LOAD_ADDR      0x08000000

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

    /* Address map params */
    cci::cci_param<cci::uint64> m_addr_map_ram;
    cci::cci_param<cci::uint64> m_addr_map_uart;

    QemuInstanceManager m_inst_mgr;
    QemuInstance &m_qemu_inst;

    sc_core::sc_vector<QemuCpuArmNeoverseN1> m_cpus;
    QemuArmGicv3 m_gic;
    Router<> m_router;
    Memory m_ram;
    Memory m_flash;
    QemuUartPl011 m_uart;

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

        if (!m_dtb_file.is_default_value()) {
            m_ram.load(m_dtb_file, DTB_LOAD_ADDR);
        }

        m_ram.load(reinterpret_cast<const uint8_t *>(bootloader_aarch64),
                   sizeof(bootloader_aarch64), 0x0);
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

        , m_addr_map_ram("addr_map_ram", 0x00000000, "")
        , m_addr_map_uart("addr_map_uart", 0xc0000000, "")

        , m_qemu_inst(m_inst_mgr.new_instance(QemuInstance::Target::AARCH64))
        , m_cpus("cpu", 4, [this] (const char *n, size_t i) { return new QemuCpuArmNeoverseN1(n, m_qemu_inst); })
        , m_gic("gic", m_qemu_inst)
        , m_router("router")
        , m_ram("ram", m_ram_size)
        , m_flash("flash", 256 * 1024 * 1024)
        , m_uart("uart", m_qemu_inst)
    {
        using tlm_utils::tlm_quantumkeeper;

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

