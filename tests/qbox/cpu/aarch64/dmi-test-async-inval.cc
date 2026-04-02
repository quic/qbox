/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <cstdio>
#include <vector>
#include <deque>

#include "test/cpu.h"
#include "test/tester/dmi_soak.h"

#include "cortex-a53.h"
#include "qemu-instance.h"

/*
 * Arm Cortex-A53 DMI async invalidation test.
 *
 * In this test, all CPUs share the same DMI region. Each CPU does some
 * read/modify/write to its dedicated 64-bits memory area, and at some point
 * invalidate the whole region.
 *
 * This test is quite stressful for the whole DMI sub-system as invalidations
 * are very common and come from every CPU randomly. Each CPU checks that the
 * value it reads from memory is the one it expects, and at the end, the test
 * checks that all dedicated memory areas contain the final value (corresponding
 * to the number of read/modify/write operations the CPUs did).
 */
class CpuArmCortexA53DmiAsyncInvalTest : public CpuArmTestBench<cpu_arm_cortexA53, CpuTesterDmiSoak>
{
public:
    static constexpr uint64_t NUM_WRITES = 10000;

    static constexpr const char* FIRMWARE = R"(
        _start:
            ldr x2, =0x%08)" PRIx64 R"(
            ldr x1, =0x%08)" PRIx64 R"(
            ldr x5, =0x%08)" PRIx64 R"(
            ldr x6, =0x%08)" PRIx64 R"(
            ldr x3, =0x%08)" PRIx64 R"(


            mrs x0, mpidr_el1

            and x10, x0, #0xff
            and x0, x0, #0xff00
            lsr x0, x0, #5
            orr  x0, x0, x10
            lsl x0, x0, #3

            add x2, x2, x0
            ror x6, x6, x0

        loop:
            mov x0, #1
            str x0, [x2]
            str x3, [x2]

            mov x10, x6
            ror x10, x10, #17
            eor x6, x6, x10
            ror x10, x10, #54
            eor x6, x6, x10
            ror x10, x10, #29
            eor x6, x6, x10
            mov x6, x10

            and x10, x10, x5

            add x10, x10, x1
            and x10, x10, #-8
            str x10, [x10]

            mov x0, #2
            str x0, [x2]
            str x10, [x2]

            ldr x7, [x10]
            cmp x10, x7
            b.ne fail

            sub x3, x3, #1
            cmp x3, #0
            b.ne loop

        end:
            wfi
            mov x0, #0
            str x0, [x2]
            b end

        fail:
            mov x0, #-2
            str x0, [x2]
            b end
    )";

private:
    /*
     * Decrease the number of write per CPU when the number of CPUs increases
     * so that the test does not last forever.
     */
    uint64_t m_num_write_per_cpu;
    std::thread m_thread;
    bool running = 0;

public:
    CpuArmCortexA53DmiAsyncInvalTest(const sc_core::sc_module_name& n)
        : CpuArmTestBench<cpu_arm_cortexA53, CpuTesterDmiSoak>(n)
    {
        char buf[2048];
        SCP_DEBUG(SCMOD) << "CpuArmCortexA53DmiAsyncInvalTest constructor";
        m_num_write_per_cpu = NUM_WRITES / (p_num_cpu * 2);

        std::snprintf(buf, sizeof(buf), FIRMWARE, CpuTesterDmiSoak::MMIO_ADDR, CpuTesterDmiSoak::DMI_ADDR,
                      CpuTesterDmiSoak::DMI_SIZE - 1, (((uint64_t)std::rand() << 32) | rand()), m_num_write_per_cpu);
        set_firmware(buf);
    }

    virtual void start_of_simulation() override
    {
        running = true;
        m_thread = std::thread([&]() { inval(); });
    }
    void inval()
    {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            uint64_t l = CpuTesterDmiSoak::DMI_SIZE;
            uint64_t s = (std::rand() + 1u) % l;
            l -= s;
            uint64_t e = s + ((std::rand() + 1u) % l);
            if (running) {
                SCP_INFO(SCMOD) << "INVALIDATING";
                m_tester.dmi_invalidate(s, e);
                SCP_INFO(SCMOD) << "Invalidation done";
            }
        }
    }

    virtual ~CpuArmCortexA53DmiAsyncInvalTest() {}

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        if (id != CpuTesterDmiSoak::SOCKET_MMIO) {
            SCP_INFO(SCMOD) << "NON DMI write data: 0x" << std::hex << data << ", len: 0x" << len;

            return;
        }

        SCP_INFO(SCMOD) << "cpu_" << cpuid << " write at 0x" << std::hex << addr << " data:0x" << data << ", len: 0x"
                        << len;
        if (data == 0) {
            SCP_INFO(SCMOD) << "cpu_" << cpuid << " DONE";
            // for CPU to shut down (accelerators may not WFI idle, and if they wake up, they may keep each other awake)
            m_cpus[cpuid].halt_cb(true);
        }
        TEST_ASSERT(data != -1);
        TEST_ASSERT(data != -2);
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override
    {
        int cpuid = addr >> 3;

        /* No read on the control socket */
        TEST_ASSERT(id == CpuTesterDmiSoak::SOCKET_DMI);
        SCP_INFO(SCMOD) << "CPU NON DMI read at 0x" << std::hex << addr;

        /* The return value is ignored by the tester */
        return 0;
    }

    virtual bool dmi_request(int id, uint64_t addr, size_t len, tlm::tlm_dmi& ret) override
    {
        SCP_INFO(SCMOD) << "DMI request at " << addr << ", len: " << len;
        return true;
    }

    virtual void end_of_simulation() override
    {
        CpuArmTestBench<cpu_arm_cortexA53, CpuTesterDmiSoak>::end_of_simulation();
        running = false;
        m_thread.join();
    }
};

constexpr const char* CpuArmCortexA53DmiAsyncInvalTest::FIRMWARE;

int sc_main(int argc, char* argv[]) { return run_testbench<CpuArmCortexA53DmiAsyncInvalTest>(argc, argv); }
