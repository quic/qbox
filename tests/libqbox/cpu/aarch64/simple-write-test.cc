/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <cstdio>
#include <vector>

#include <cci/utils/broker.h>
#include <libgsutils.h>

#include "test/cpu.h"
#include "test/tester/mmio.h"

#include "cortex-a53.h"
#include "qemu-instance.h"

/*
 * Simple ARM Cortex-A53 write test.
 *
 * The CPU starts by fetching its SMP affinity number, then writes ten times to
 * the address (i * 8) of the tester, with i == affinity num, incrementing the
 * value each time. Once it reaches 10, it goes into sleep by calling wfi,
 * effectively starving the kernel and ending the simulation.
 *
 * On each write, the test bench checks the written value. It also checks the
 * number of write at the end of the simulation.
 */
class CpuArmCortexA53SimpleWriteTest : public CpuTestBench<cpu_arm_cortexA53, CpuTesterMmio>
{
public:
    static constexpr int NUM_WRITES = 10;

    static constexpr const char* FIRMWARE = R"(
        _start:
            ldr x1, =0x%08)" PRIx64 R"(

            mrs x0, mpidr_el1

            and x2, x0, #0xff
            and x0, x0, #0xff00
            lsr x0, x0, #5
            orr  x0, x0, x2

            lsl x0, x0, #3
            add x1, x1, x0

            mov x0, #0

        loop:
            str x0, [x1]
            add x0, x0, #1
            cmp x0, #%d
            b.ne loop

        end:
            wfi
            b end
    )";

protected:
    std::vector<int> m_writes;

public:
    CpuArmCortexA53SimpleWriteTest(const sc_core::sc_module_name& n)
        : CpuTestBench<cpu_arm_cortexA53, CpuTesterMmio>(n)
    {
        char buf[1024];

        std::snprintf(buf, sizeof(buf), FIRMWARE, CpuTesterMmio::MMIO_ADDR, NUM_WRITES);
        set_firmware(buf);

        m_writes.resize(p_num_cpu);
        for (int i = 0; i < p_num_cpu; i++) {
            m_writes[i] = 0;
        }
    }

    virtual ~CpuArmCortexA53SimpleWriteTest() {}

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "CPU write at 0x" << std::hex << addr << ", data: " << std::hex << data;

        TEST_ASSERT(cpuid < p_num_cpu);
        TEST_ASSERT(data == m_writes[cpuid]);

        m_writes[cpuid]++;
    }

    virtual void end_of_simulation() override
    {
        CpuTestBench<cpu_arm_cortexA53, CpuTesterMmio>::end_of_simulation();

        for (int i = 0; i < p_num_cpu; i++) {
            TEST_ASSERT(m_writes[i] == NUM_WRITES);
        }
    }
};

constexpr const char* CpuArmCortexA53SimpleWriteTest::FIRMWARE;

int sc_main(int argc, char* argv[]) { return run_testbench<CpuArmCortexA53SimpleWriteTest>(argc, argv); }
