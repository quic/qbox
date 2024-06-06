/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdio>
#include <vector>
#include <deque>

#include "test/cpu.h"
#include "test/tester/exclusive.h"

#include "cortex-a53.h"
#include "qemu-instance.h"

/*
 * Arm Cortex-A53 Load/Store exclusive pair failure.
 *
 * Each CPU tries to lock a memory region which has been pre-locked at the
 * beginning of the test. The corresponding stxr should fail in all cases.
 */
class CpuArmCortexA53LdStExclTest : public CpuTestBench<cpu_arm_cortexA53, CpuTesterExclusive>
{
    bool passed = true;

public:
    static constexpr const char* FIRMWARE = R"(
        _start:
            ldr x1, =0x%08)" PRIx64 R"(

        loop:
            ldxr x0, [x1]
            stxr w2, x0, [x1]

            # Make sure the exclusive store failed
            cmp w2, #0
            beq fail

        end:
            wfi
            b end

        fail:
            str x0, [x1]
            b end
    )";

public:
    SC_HAS_PROCESS(CpuArmCortexA53LdStExclTest);

    CpuArmCortexA53LdStExclTest(const sc_core::sc_module_name& n)
        : CpuTestBench<cpu_arm_cortexA53, CpuTesterExclusive>(n)
    {
        char buf[2048];

        std::snprintf(buf, sizeof(buf), FIRMWARE, CpuTesterExclusive::MMIO_ADDR);
        set_firmware(buf);
    }

    virtual ~CpuArmCortexA53LdStExclTest() {}

    virtual void end_of_elaboration() override { m_tester.lock_region_64(0); }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "FAILED : CPU " << cpuid << " write, data: " << std::hex << data << ", len: " << len;
        passed = false;
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override { return 0; }

    virtual void end_of_simulation() override
    {
        TEST_ASSERT(passed);
        CpuTestBench<cpu_arm_cortexA53, CpuTesterExclusive>::end_of_simulation();
    }
};

constexpr const char* CpuArmCortexA53LdStExclTest::FIRMWARE;

int sc_main(int argc, char* argv[]) { return run_testbench<CpuArmCortexA53LdStExclTest>(argc, argv); }
