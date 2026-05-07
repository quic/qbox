/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * SMMUv3 Test Bench
 *
 * Uses the qbox TestBench framework so SystemC is re-initialized per test
 * (see systemc-components/common/include/tests/test-bench.h).
 *
 * Wires:
 *   router.target_socket       <- tests' mmio_initiator (MMIO + DRAM access)
 *   router.initiator_socket    -> smmu.socket            (MMIO register file)
 *   router.initiator_socket    -> main_mem.socket        (DRAM)
 *   tbu.upstream_socket        <- tests' tbu_initiator   (issues translated txns)
 *   tbu.downstream_socket      -> router.target_socket   (after translation)
 *   smmu.dma_socket            -> router.target_socket   (walks, queues, events)
 */
#ifndef SMMUV3_BENCH_H
#define SMMUV3_BENCH_H

#include <tests/test-bench.h>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <cci_configuration>
#include <gs_memory.h>
#include <router.h>
#include "smmuv3.h"

static constexpr uint64_t SMMUV3_MMIO_BASE = 0x15000000ULL;
static constexpr uint64_t SMMUV3_MMIO_SIZE = 0x20000ULL; // 128KB
static constexpr uint64_t DRAM_BASE = 0x80000000ULL;
static constexpr uint64_t DRAM_SIZE = 0x10000000ULL; // 256MB

class smmuv3_bench : public TestBench
{
private:
    // Helper struct constructed BEFORE smmu (via member declaration order)
    // so CCI presets are in the broker by the time smmu's json_module reads
    // its target_socket.address from there.
    struct preset_setter {
        preset_setter(const std::string& bench_name) { set_presets(bench_name); }
    };

public:
    preset_setter _presets; // MUST be first member
    gs::smmuv3<32> smmu;
    gs::smmuv3_tbu<32> tbu;
    gs::gs_memory<> main_mem;
    gs::router<> router;

    tlm_utils::simple_initiator_socket<smmuv3_bench> tbu_initiator;
    tlm_utils::simple_initiator_socket<smmuv3_bench> mmio_initiator;

    // Helper: set the CCI presets for a bench of a given top-level name.
    // Must be called BEFORE the bench is instantiated so the smmu's
    // internal json_module can read target_socket address/size at construction.
    static void set_presets(const std::string& bench_name)
    {
        // We may be called from inside the SystemC hierarchy (bench ctor),
        // so use cci_get_broker() not cci_get_global_broker().
        auto broker = cci::cci_get_broker();
        broker.set_preset_cci_value(bench_name + ".smmu.target_socket.address", cci::cci_value(SMMUV3_MMIO_BASE));
        broker.set_preset_cci_value(bench_name + ".smmu.target_socket.size", cci::cci_value(SMMUV3_MMIO_SIZE));
        // smmu's internal json_module/reg_router expects RELATIVE addresses
        // (its map shows "Address map ... at address 0x0 size 0x20000 (with
        // relative address)"). So the outer router must subtract the base
        // before forwarding -> relative_addresses=true.
        broker.set_preset_cci_value(bench_name + ".smmu.target_socket.relative_addresses", cci::cci_value(true));
        broker.set_preset_cci_value(bench_name + ".main_mem.target_socket.address", cci::cci_value(DRAM_BASE));
        broker.set_preset_cci_value(bench_name + ".main_mem.target_socket.size", cci::cci_value(DRAM_SIZE));
        broker.set_preset_cci_value(bench_name + ".main_mem.target_socket.relative_addresses", cci::cci_value(false));
    }

    smmuv3_bench(const sc_core::sc_module_name& n)
        : TestBench(n)
        , _presets(static_cast<const char*>(n))
        , smmu("smmu")
        , tbu("tbu", &smmu)
        , main_mem("main_mem")
        , router("router")
        , tbu_initiator("tbu_initiator")
        , mmio_initiator("mmio_initiator")
    {
        // Wire the fabric
        router.initiator_socket.bind(smmu.socket);
        router.initiator_socket.bind(main_mem.socket);
        tbu_initiator.bind(tbu.upstream_socket);
        tbu.downstream_socket.bind(router.target_socket);
        smmu.dma_socket.bind(router.target_socket);
        mmio_initiator.bind(router.target_socket);
    }

    // ---- test-body helpers ------------------------------------------

    void write_dram(uint64_t addr, const void* data, size_t len)
    {
        tlm::tlm_generic_payload txn;
        txn.set_command(tlm::TLM_WRITE_COMMAND);
        txn.set_address(addr);
        txn.set_data_ptr(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(data)));
        txn.set_data_length(len);
        txn.set_streaming_width(len);
        txn.set_byte_enable_length(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        mmio_initiator->b_transport(txn, delay);
    }

    void read_dram(uint64_t addr, void* data, size_t len)
    {
        tlm::tlm_generic_payload txn;
        txn.set_command(tlm::TLM_READ_COMMAND);
        txn.set_address(addr);
        txn.set_data_ptr(reinterpret_cast<unsigned char*>(data));
        txn.set_data_length(len);
        txn.set_streaming_width(len);
        txn.set_byte_enable_length(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        mmio_initiator->b_transport(txn, delay);
    }

    tlm::tlm_response_status mmio_write32(uint64_t offset, uint32_t value)
    {
        tlm::tlm_generic_payload txn;
        txn.set_command(tlm::TLM_WRITE_COMMAND);
        txn.set_address(SMMUV3_MMIO_BASE + offset);
        txn.set_data_ptr(reinterpret_cast<unsigned char*>(&value));
        txn.set_data_length(4);
        txn.set_streaming_width(4);
        txn.set_byte_enable_length(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        mmio_initiator->b_transport(txn, delay);
        return txn.get_response_status();
    }

    uint32_t mmio_read32(uint64_t offset)
    {
        uint32_t value = 0;
        tlm::tlm_generic_payload txn;
        txn.set_command(tlm::TLM_READ_COMMAND);
        txn.set_address(SMMUV3_MMIO_BASE + offset);
        txn.set_data_ptr(reinterpret_cast<unsigned char*>(&value));
        txn.set_data_length(4);
        txn.set_streaming_width(4);
        txn.set_byte_enable_length(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        mmio_initiator->b_transport(txn, delay);
        return value;
    }

    tlm::tlm_response_status tbu_txn(uint64_t iova, bool write, uint32_t data)
    {
        tlm::tlm_generic_payload txn;
        txn.set_command(write ? tlm::TLM_WRITE_COMMAND : tlm::TLM_READ_COMMAND);
        txn.set_address(iova);
        uint32_t local = data;
        txn.set_data_ptr(reinterpret_cast<unsigned char*>(&local));
        txn.set_data_length(4);
        txn.set_streaming_width(4);
        txn.set_byte_enable_length(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        tbu_initiator->b_transport(txn, delay);
        return txn.get_response_status();
    }

    // ---- translation-structure helpers ------------------------------

    void setup_linear_stream_table(uint64_t base_addr, uint32_t log2size)
    {
        mmio_write32(0x80, static_cast<uint32_t>(base_addr));
        mmio_write32(0x84, static_cast<uint32_t>(base_addr >> 32));
        mmio_write32(0x88, log2size & 0x3F); // FMT=0 linear, LOG2SIZE
    }

    void setup_2level_stream_table(uint64_t base_addr, uint32_t log2size, uint32_t split)
    {
        mmio_write32(0x80, static_cast<uint32_t>(base_addr));
        mmio_write32(0x84, static_cast<uint32_t>(base_addr >> 32));
        mmio_write32(0x88, (log2size & 0x3F) | ((split & 0x1F) << 6) | (1u << 16));
    }

    void enable_smmu()
    {
        mmio_write32(0x20, mmio_read32(0x20) | 0x1);
        sc_core::wait(1, sc_core::SC_NS);
    }

    void enable_cmdq()
    {
        mmio_write32(0x20, mmio_read32(0x20) | (1u << 3));
        sc_core::wait(1, sc_core::SC_NS);
    }

    void write_ste_bypass(uint64_t ste_addr)
    {
        uint8_t ste[64] = { 0 };
        uint64_t* dw = reinterpret_cast<uint64_t*>(ste);
        dw[0] = 0x1ULL | (4ULL << 1);
        write_dram(ste_addr, ste, 64);
    }

    // V, CONFIG, S1ContextPtr, S1CDMax all live in DW0 per the SMMUv3 spec.
    void write_ste_s1(uint64_t ste_addr, uint64_t cd_ptr)
    {
        uint8_t ste[64] = { 0 };
        uint64_t* dw = reinterpret_cast<uint64_t*>(ste);
        dw[0] = 0x1ULL | (5ULL << 1) | (cd_ptr & 0x000FFFFFFFFFFFC0ULL);
        write_dram(ste_addr, ste, 64);
    }

    // CD layout (matches gs::smmuv3<>::read_cd()):
    //   word[0]: T0SZ[5:0], TG0[7:6], EPD0[14], ENDI[15],
    //            T1SZ[21:16], TG1[23:22], EPD1[30], VALID[31]
    //   word[1]: IPS[2:0], AFFD[3], TBI[7:6], AARCH64[9], HD[10], HA[11],
    //            S[12], R[13], A[14], ASID[31:16]
    //   word[2]: NSCFG0[0], HAD0[1], TTB0_LO[31:4]
    //   word[3]: TTB0_HI[19:0]
    //   word[4]: NSCFG1[0], HAD1[1], TTB1_LO[31:4]
    //   word[5]: TTB1_HI[19:0]
    void write_cd_identity(uint64_t cd_addr, uint64_t ttb, uint32_t tsz, uint32_t tg, uint32_t ips)
    {
        uint8_t cd[64] = { 0 };
        uint32_t* w = reinterpret_cast<uint32_t*>(cd);

        // word[0]: T0SZ in [5:0], TG0 in [7:6], VALID in [31], EPD1 set so
        // TTBR1 path is disabled (we only configure TTBR0).
        w[0] = (tsz & 0x3F) | ((tg & 0x3) << 6) | (1u << 30) // EPD1=1 (disable TTBR1)
               | (1u << 31);                                 // VALID=1
        // word[1]: IPS in [2:0], AARCH64 in [9]
        w[1] = (ips & 0x7) | (1u << 9);
        // word[2]: TTB0_LO bits [31:4]
        w[2] = static_cast<uint32_t>(ttb) & 0xFFFFFFF0u;
        // word[3]: TTB0_HI bits [19:0] (i.e. TTB[51:32])
        w[3] = static_cast<uint32_t>(ttb >> 32) & 0xFFFFFu;

        write_dram(cd_addr, cd, 64);
    }

    void write_table_desc(uint64_t table_addr, uint32_t idx, uint64_t next_table)
    {
        uint64_t desc = (next_table & 0x0000FFFFFFFFF000ULL) | 0x3;
        write_dram(table_addr + idx * 8, &desc, 8);
    }

    // LPAE block/page descriptor flags:
    //   bit 0  = type[0] (1 for valid)
    //   bit 1  = type[1] (1 for table/page, 0 for block at non-leaf level)
    //   bit 6  = AP[1]   (1 = EL0 access allowed; required by walker)
    //   bit 10 = AF      (Access Flag, must be set)
    static constexpr uint64_t DESC_FLAGS_BLOCK = (1ULL << 0) | (1ULL << 6) | (1ULL << 10);
    static constexpr uint64_t DESC_FLAGS_PAGE = (1ULL << 0) | (1ULL << 1) | (1ULL << 6) | (1ULL << 10);

    // 1GB block at L1 (4KB granule). PA must be 1GB-aligned.
    void write_block_1gb(uint64_t table_addr, uint32_t idx, uint64_t pa)
    {
        uint64_t desc = (pa & ~0x3FFFFFFFULL) | DESC_FLAGS_BLOCK;
        write_dram(table_addr + idx * 8, &desc, 8);
    }

    // 2MB block at L2 (4KB granule). PA must be 2MB-aligned.
    void write_block_2mb(uint64_t table_addr, uint32_t idx, uint64_t pa)
    {
        uint64_t desc = (pa & ~0x1FFFFFULL) | DESC_FLAGS_BLOCK;
        write_dram(table_addr + idx * 8, &desc, 8);
    }

    // 4KB page at L3 (4KB granule). PA must be 4KB-aligned.
    void write_page_4k(uint64_t table_addr, uint32_t idx, uint64_t pa)
    {
        uint64_t desc = (pa & ~0xFFFULL) | DESC_FLAGS_PAGE;
        write_dram(table_addr + idx * 8, &desc, 8);
    }

    void setup_cmdq(uint64_t base_addr, uint32_t log2size)
    {
        uint32_t lo = static_cast<uint32_t>(base_addr) | (log2size & 0x1F);
        uint32_t hi = static_cast<uint32_t>(base_addr >> 32);
        mmio_write32(0x90, lo);
        mmio_write32(0x94, hi);
        mmio_write32(0x98, 0); // PROD
        mmio_write32(0x9C, 0); // CONS
    }

    void issue_cmd(uint64_t cmdq_base, uint32_t idx, const uint8_t cmd[16])
    {
        write_dram(cmdq_base + idx * 16, cmd, 16);
        mmio_write32(0x98, idx + 1);
        sc_core::wait(10, sc_core::SC_NS);
    }
};

#endif // SMMUV3_BENCH_H
