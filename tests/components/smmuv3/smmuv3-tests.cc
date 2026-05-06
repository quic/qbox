/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * SMMUv3 Unit Tests
 *
 * Uses the qbox TEST_BENCH(name, TestCase) pattern - each test re-forks
 * SystemC so broker/module state is fresh.
 */

#include "smmuv3-bench.h"

// SMMUv3 command opcodes
#define CMD_CFGI_STE_RANGE 0x04
#define CMD_TLBI_NH_ALL    0x10
#define CMD_TLBI_NH_ASID   0x11
#define CMD_SYNC           0x46

// Page 0 register offsets
#define REG_IDR0          0x000
#define REG_IDR1          0x004
#define REG_IDR5          0x014
#define REG_IIDR          0x018
#define REG_CR0           0x020
#define REG_CR0ACK        0x024
#define REG_IRQ_CTRL      0x050
#define REG_IRQ_CTRL_ACK  0x054
#define REG_CMDQ_PROD     0x098
#define REG_CMDQ_CONS     0x09C
#define REG_EVENTQ_PROD   0x0A8
#define REG_EVENTQ_CONS   0x0AC
#define REG_GATOS_CTRL    0x100
#define REG_GATOS_SID     0x108
#define REG_GATOS_ADDR_LO 0x110
#define REG_GATOS_PAR_LO  0x118

// Page 1 aliases (Page 0 offset + 0x10000)
#define REG_P1_CMDQ_PROD   (0x10000 + 0x098)
#define REG_P1_EVENTQ_PROD (0x10000 + 0x0A8)
#define REG_P1_EVENTQ_CONS (0x10000 + 0x0AC)

// ------------------------------------------------------------------
// Register sanity
// ------------------------------------------------------------------

TEST_BENCH(smmuv3_bench, ID_Registers)
{
    uint32_t idr0 = mmio_read32(REG_IDR0);
    ASSERT_TRUE(idr0 & 0x1);           // S2P
    ASSERT_TRUE(idr0 & 0x2);           // S1P
    ASSERT_EQ((idr0 >> 2) & 0x3, 0x2); // TTF=2 (AArch64)

    uint32_t idr1 = mmio_read32(REG_IDR1);
    ASSERT_GT(idr1 & 0x3F, 0u); // SIDSIZE

    uint32_t idr5 = mmio_read32(REG_IDR5);
    ASSERT_NE(idr5 & 0x7, 0u);      // OAS
    ASSERT_TRUE((idr5 >> 4) & 0x1); // GRAN4K

    uint32_t iidr = mmio_read32(REG_IIDR);
    ASSERT_EQ(iidr, 0x43B20001u);
}

TEST_BENCH(smmuv3_bench, CR0_CR0ACK_Mirror)
{
    mmio_write32(REG_CR0, 0x1);
    sc_core::wait(1, sc_core::SC_NS);
    ASSERT_EQ(mmio_read32(REG_CR0ACK), 0x1u);

    mmio_write32(REG_CR0, 0x1 | (1u << 3));
    sc_core::wait(1, sc_core::SC_NS);
    ASSERT_EQ(mmio_read32(REG_CR0ACK), 0x1u | (1u << 3));
}

TEST_BENCH(smmuv3_bench, IRQ_CTRL_ACK_Mirror)
{
    mmio_write32(REG_IRQ_CTRL, 0x7);
    sc_core::wait(1, sc_core::SC_NS);
    ASSERT_EQ(mmio_read32(REG_IRQ_CTRL_ACK), 0x7u);
}

// ------------------------------------------------------------------
// Translation paths
// ------------------------------------------------------------------

// With T0SZ=16 (48-bit IOVA) and 4KB granule, the walker does a full 4-level
// walk: L0 -> L1 -> L2 -> L3. Map VA 0 -> some 4KB-aligned PA in DRAM.
TEST_BENCH(smmuv3_bench, LinearStreamTable_S1_4KPage)
{
    const uint64_t strtab = DRAM_BASE;
    const uint64_t cd_base = DRAM_BASE + 0x1000;
    const uint64_t l0_table = DRAM_BASE + 0x2000;
    const uint64_t l1_table = DRAM_BASE + 0x3000;
    const uint64_t l2_table = DRAM_BASE + 0x4000;
    const uint64_t l3_table = DRAM_BASE + 0x5000;
    const uint64_t page_pa = DRAM_BASE + 0x08000000; // 128MB into 256MB DRAM, 4K-aligned

    setup_linear_stream_table(strtab, 4);
    write_ste_s1(strtab, cd_base);
    write_cd_identity(cd_base, l0_table, 16, 0, 5);
    write_table_desc(l0_table, 0, l1_table);
    write_table_desc(l1_table, 0, l2_table);
    write_table_desc(l2_table, 0, l3_table);
    write_page_4k(l3_table, 0, page_pa);

    enable_smmu();

    ASSERT_EQ(tbu_txn(0x0, true, 0xDEADBEEFu), tlm::TLM_OK_RESPONSE);

    uint32_t readback = 0;
    read_dram(page_pa, &readback, 4);
    ASSERT_EQ(readback, 0xDEADBEEFu);
}

// Using a 2MB block at L2 instead of a 4KB page at L3. With 4KB granule,
// firstblocklevel=1 so blocks are valid at L1 (1GB) and L2 (2MB).
// Use a 2MB-aligned dest_pa (the block-aligned PA must equal what we read back).
TEST_BENCH(smmuv3_bench, LinearStreamTable_S1_2MBlock)
{
    const uint64_t strtab = DRAM_BASE;
    const uint64_t cd_base = DRAM_BASE + 0x1000;
    const uint64_t l0_table = DRAM_BASE + 0x2000;
    const uint64_t l1_table = DRAM_BASE + 0x3000;
    const uint64_t l2_table = DRAM_BASE + 0x4000;
    const uint64_t dest_pa = DRAM_BASE + 0x00200000; // 2MB-aligned

    setup_linear_stream_table(strtab, 4);
    write_ste_s1(strtab, cd_base);
    write_cd_identity(cd_base, l0_table, 16, 0, 5);
    write_table_desc(l0_table, 0, l1_table);
    write_table_desc(l1_table, 0, l2_table);
    write_block_2mb(l2_table, 0, dest_pa);

    enable_smmu();

    ASSERT_EQ(tbu_txn(0x0, true, 0xCAFEBABE), tlm::TLM_OK_RESPONSE);

    uint32_t readback = 0;
    read_dram(dest_pa, &readback, 4);
    ASSERT_EQ(readback, 0xCAFEBABEu);
}

// ------------------------------------------------------------------
// Command queue
// ------------------------------------------------------------------

TEST_BENCH(smmuv3_bench, CMDQ_Sync_UpdatesCons)
{
    const uint64_t cmdq_base = DRAM_BASE + 0x00020000;

    setup_cmdq(cmdq_base, 4);
    enable_cmdq();

    uint8_t cmd[16] = { 0 };
    cmd[0] = CMD_SYNC;
    issue_cmd(cmdq_base, 0, cmd);

    ASSERT_EQ(mmio_read32(REG_CMDQ_CONS) & 0x1F, 1u);
}

TEST_BENCH(smmuv3_bench, CMDQ_TLBI_NH_ALL)
{
    const uint64_t cmdq_base = DRAM_BASE + 0x00020000;

    setup_cmdq(cmdq_base, 4);
    enable_cmdq();

    uint8_t cmd[16] = { 0 };
    cmd[0] = CMD_TLBI_NH_ALL;
    issue_cmd(cmdq_base, 0, cmd);

    ASSERT_EQ(mmio_read32(REG_CMDQ_CONS) & 0x1F, 1u);
}

// ------------------------------------------------------------------
// Page 1 aliasing
// ------------------------------------------------------------------

TEST_BENCH(smmuv3_bench, Page1_Aliases_CMDQ_PROD)
{
    const uint64_t cmdq_base = DRAM_BASE + 0x00020000;

    setup_cmdq(cmdq_base, 4);
    enable_cmdq();

    uint8_t cmd[16] = { 0 };
    cmd[0] = CMD_SYNC;
    write_dram(cmdq_base, cmd, 16);

    // Write to Page 1 CMDQ_PROD instead of Page 0
    mmio_write32(REG_P1_CMDQ_PROD, 1);
    sc_core::wait(10, sc_core::SC_NS);

    // Page 0 CMDQ_PROD should reflect Page 1 write; CMDQ_CONS should advance.
    ASSERT_EQ(mmio_read32(REG_CMDQ_PROD), 1u);
    ASSERT_EQ(mmio_read32(REG_CMDQ_CONS) & 0x1F, 1u);
}

TEST_BENCH(smmuv3_bench, Page1_Aliases_EVENTQ_CONS)
{
    mmio_write32(REG_P1_EVENTQ_CONS, 0x42);
    sc_core::wait(1, sc_core::SC_NS);
    ASSERT_EQ(mmio_read32(REG_EVENTQ_CONS), 0x42u);
    ASSERT_EQ(mmio_read32(REG_P1_EVENTQ_CONS), 0x42u);
}

// ------------------------------------------------------------------
// GATOS
// ------------------------------------------------------------------

TEST_BENCH(smmuv3_bench, GATOS_Translation)
{
    const uint64_t strtab = DRAM_BASE;
    const uint64_t cd_base = DRAM_BASE + 0x1000;
    const uint64_t l0_table = DRAM_BASE + 0x2000;
    const uint64_t l1_table = DRAM_BASE + 0x3000;
    const uint64_t l2_table = DRAM_BASE + 0x4000;
    const uint64_t l3_table = DRAM_BASE + 0x5000;
    const uint64_t page_pa = DRAM_BASE + 0x10000000; // 4K-aligned

    setup_linear_stream_table(strtab, 4);
    write_ste_s1(strtab, cd_base);
    write_cd_identity(cd_base, l0_table, 16, 0, 5);
    write_table_desc(l0_table, 0, l1_table);
    write_table_desc(l1_table, 0, l2_table);
    write_table_desc(l2_table, 0, l3_table);
    write_page_4k(l3_table, 0, page_pa);

    enable_smmu();

    // Translate IOVA 0 (which lands in the page mapped to page_pa)
    mmio_write32(REG_GATOS_SID, 0);
    mmio_write32(REG_GATOS_ADDR_LO, 0x0);
    mmio_write32(REG_GATOS_ADDR_LO + 4, 0);
    mmio_write32(REG_GATOS_CTRL, (1u << 8) | (1u << 0)); // READ_NWRITE=1, RUN=1
    sc_core::wait(10, sc_core::SC_NS);

    uint32_t par_lo = mmio_read32(REG_GATOS_PAR_LO);
    ASSERT_EQ(par_lo & 0x1, 0u); // F=0 (no fault)
    ASSERT_EQ(par_lo & ~0xFFFu, static_cast<uint32_t>(page_pa) & ~0xFFFu);
}

int sc_main(int argc, char** argv)
{
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);
    // Presets are set per-bench via the smmuv3_bench::_presets member
    // (which runs inside the SystemC hierarchy where cci_get_broker is valid).
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
