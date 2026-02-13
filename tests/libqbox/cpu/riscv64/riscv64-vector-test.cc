/*
 * This file is part of libqbox
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <iostream>
#include <fstream>
#include <vector>
#include <type_traits>

#include <cci_configuration>
#include <cci/utils/broker.h>

#include "riscv64.h"
#include "test/cpu.h"
#include "test/tester/tester.h"
#include "test/tester/mmio.h"
#include "test/test.h"
#include "qemu-instance.h"

#include <scp/report.h>

/*
 * RISC-V 64-bit Vector ISA test.
 *
 * This test loads a vector program and uses a reset mechanism to start CPU execution.
 * The program performs vector operations and writes results to memory for verification.
 */
template <class CPU, class TESTER>
class CpuRiscvTestBench : public CpuTestBench<CPU, TESTER>
{
public:
    CpuRiscvTestBench(const sc_core::sc_module_name& n): CpuTestBench<CPU, TESTER>(n)
    {
        int i = 0;
        for (CPU& cpu : CpuTestBench<CPU, TESTER>::m_cpus) {
            cpu.p_hartid = i++;
            // Enable vector extension for this test
            if constexpr (std::is_same_v<CPU, cpu_riscv64>) {
                cpu.p_vector = true;
            }
        }
    }
};

class Riscv64VectorTest : public CpuRiscvTestBench<cpu_riscv64, CpuTesterMmio>
{
protected:
    static constexpr int EXPECTED_WRITES = 1;          // We expect one completion write
    static constexpr int EXPECTED_VECTOR_ELEMENTS = 4; // We expect 4 vector element writes
    int m_completion_writes = 0;
    int m_vector_element_writes = 0;
    std::vector<uint32_t> m_vector_results;
    bool m_test_completed = false;

    void load_vector_program()
    {
        // Load the assembled RISC-V vector program from binary file
        std::ifstream file(FIRMWARE_BIN_PATH, std::ios::binary | std::ios::ate);
        if (!file) {
            SCP_FATAL(SCMOD) << "Failed to open firmware binary: " << FIRMWARE_BIN_PATH;
            TEST_ASSERT(false);
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> program(size);
        if (!file.read(reinterpret_cast<char*>(program.data()), size)) {
            SCP_FATAL(SCMOD) << "Failed to read firmware binary";
            TEST_ASSERT(false);
        }

        // Load program into memory at address 0x0 (reset vector)
        m_mem.load.ptr_load(program.data(), MEM_ADDR, size);

        SCP_INFO(SCMOD) << "Loaded RISC-V vector program at 0x" << std::hex << MEM_ADDR << ", size=" << std::dec << size
                        << " bytes";

        // Invalidate translation block cache
        m_inst_a.get().tb_invalidate_phys_range(MEM_ADDR, size);
        m_inst_b.get().tb_invalidate_phys_range(MEM_ADDR, size);
    }

public:
    Riscv64VectorTest(const sc_core::sc_module_name& n): CpuRiscvTestBench<cpu_riscv64, CpuTesterMmio>(n)
    {
        // Configure CPU to start at address 0x0
        for (auto& cpu : m_cpus) {
            cpu.p_resetvec = MEM_ADDR;
        }

        // Load the vector test program
        load_vector_program();

        // Initialize vector results storage
        m_vector_results.resize(EXPECTED_VECTOR_ELEMENTS, 0);
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        SCP_INFO(SCMOD) << "MMIO write: addr=0x" << std::hex << addr << ", data=0x" << data;

        // Check if this is a completion signal (addr 0x0 relative to MMIO base)
        if (addr == 0x0) {
            if (data == 0x600DCAFE) {
                m_completion_writes++;
                m_test_completed = true;
                SCP_INFO(SCMOD) << "Vector test completed successfully, stopping simulation";
                sc_core::sc_stop();
            } else if (data == 0xDEADBEEF) {
                SCP_FATAL(SCMOD) << "Vector test failed - assembly reported error";
                TEST_ASSERT(false);
            }
        }
        // Check if this is vector result data (addr 0x4 + offset relative to MMIO base)
        else if (addr >= 0x4 && addr < 0x4 + (EXPECTED_VECTOR_ELEMENTS * 4)) {
            int index = (addr - 0x4) / 4;
            if (index < EXPECTED_VECTOR_ELEMENTS) {
                m_vector_results[index] = static_cast<uint32_t>(data);
                m_vector_element_writes++;
                SCP_INFO(SCMOD) << "Vector element[" << index << "] = " << data;
            }
        }
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override { return 0; }

    virtual void end_of_simulation() override
    {
        CpuTestBench<cpu_riscv64, CpuTesterMmio>::end_of_simulation();

        if (!m_test_completed) {
            SCP_FATAL(SCMOD) << "Test did not complete - CPU may not have started execution";
            TEST_ASSERT(false);
        }

        // Verify the expected results were received via MMIO
        SCP_INFO(SCMOD) << "Verifying vector computation results received via MMIO...";

        // Check vector addition results: should be [11, 22, 33, 44]
        uint32_t expected_results[] = { 11, 22, 33, 44 };
        bool results_correct = true;

        // Verify we received all expected vector elements
        TEST_ASSERT(m_vector_element_writes == EXPECTED_VECTOR_ELEMENTS);

        for (int i = 0; i < EXPECTED_VECTOR_ELEMENTS; i++) {
            if (m_vector_results[i] != expected_results[i]) {
                SCP_FATAL(SCMOD) << "Vector result[" << i << "] = " << m_vector_results[i] << ", expected "
                                 << expected_results[i];
                results_correct = false;
            } else {
                SCP_INFO(SCMOD) << "Vector result[" << i << "] = " << m_vector_results[i] << " (correct)";
            }
        }

        TEST_ASSERT(results_correct);
        TEST_ASSERT(m_completion_writes == EXPECTED_WRITES);

        SCP_INFO(SCMOD) << "Vector test PASSED - all vector instructions executed correctly!";
    }
};

int sc_main(int argc, char* argv[]) { return run_testbench<Riscv64VectorTest>(argc, argv); }
