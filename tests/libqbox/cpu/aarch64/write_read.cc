/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <cstdio>
#include <vector>
#include <scp/report.h>
#include <cci/utils/broker.h>
#include <libgsutils.h>
#include <cciutils.h>
#include "test/cpu.h"
#include "test/tester/mmio.h"

#include "cortex-a53.h"
#include "qemu-instance.h"

/*
 *  ARM Cortex-A53 write/read test.
 *
 * The CPU starts by fetching its SMP affinity number, then writes ten times to
 * the address (i * 8) of the tester, with i == affinity num, incrementing the
 * value each time. Once it reaches 10, it goes into sleep by calling wfi,
 * effectively starving the kernel and ending the simulation.
 *
 * On each write, the test bench checks the written value. It also checks the
 * number of write at the end of the simulation.
 */
class CpuArmCortexA53WriteReadTest : public CpuArmTestBench<cpu_arm_cortexA53, CpuTesterMmio>
{
public:
    static constexpr uint64_t NUM_WRITES = 10 * 1024;

    static constexpr const char* FIRMWARE = R"(
        _start:
            ldr x1, =0x%08)" PRIx64 R"(
            ldr x3, =0x%08)" PRIx64 R"(
            ldr x10,=0x%08)" PRIx64 R"(

            mrs x0, mpidr_el1

            and x2, x0, #0xff
            and x0, x0, #0xff00
            lsr x0, x0, #5
            orr  x0, x0, x2

            lsl x0, x0, #3
            add x1, x1, x0

            mov x0, #0
            mov x9, x3
        loop:
            str x0, [x9]
            add x9, x9, #8
            add x0, x0, #1
            cmp x0, x10
            b.ne loop
       
            mov x0, #0
            mov x9, x3
        loop2:
            ldr x4, [x9]
            cmp x0, x4
            b.ne fail
            add x9, x9, #8
            add x0, x0, #1
            cmp x0, x10
            b.ne loop2

        end:
            mov x0, #0
            str x0, [x1]
            wfi
            b end

        fail:
            str x9, [x1]
            str x0, [x1]
            str x4, [x1]
            mov x0, #-1
            str x0, [x1]
            b end
    )";

protected:
public:
    CpuArmCortexA53WriteReadTest(const sc_core::sc_module_name& n): CpuArmTestBench<cpu_arm_cortexA53, CpuTesterMmio>(n)
    {
        char buf[1024];

        std::snprintf(buf, sizeof(buf), FIRMWARE, CpuTesterMmio::MMIO_ADDR, CpuTestBench::BULKMEM_ADDR,
                      NUM_WRITES / p_num_cpu);
        set_firmware(buf);
    }

    virtual ~CpuArmCortexA53WriteReadTest() {}

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        if (data == -1) {
            SCP_WARN(SCMOD) << "An Error was reported by the CPU";
            TEST_ASSERT(false);
        }

        SCP_INFO(SCMOD) << "CPU write at 0x" << std::hex << addr << ", data: " << std::hex << data;
    }

    virtual void end_of_simulation() override
    {
        CpuArmTestBench<cpu_arm_cortexA53, CpuTesterMmio>::end_of_simulation();
    }
};

constexpr const char* CpuArmCortexA53WriteReadTest::FIRMWARE;

int sc_main(int argc, char* argv[]) { return run_testbench<CpuArmCortexA53WriteReadTest>(argc, argv); }
