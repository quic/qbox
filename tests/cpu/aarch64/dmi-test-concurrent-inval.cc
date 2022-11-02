/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs SAS
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

#include <cstdio>
#include <vector>
#include <deque>

#include "test/cpu.h"
#include "test/tester/dmi.h"

#include "libqbox/components/cpu/arm/cortex-a53.h"
#include "libqbox/qemu-instance.h"

/*
 * Arm Cortex-A53 DMI concurrent invalidation test.
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
class CpuArmCortexA53DmiConcurrentInvalTest : public CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmi>
{
public:
    static constexpr int NUM_WRITES = 1024;

    static constexpr const char* FIRMWARE = R"(
        _start:
            ldr x2, =0x%08)" PRIx64 R"(
            ldr x1, =0x%08)" PRIx64 R"(

            mrs x0, mpidr_el1

            and x3, x0, #0xff
            and x0, x0, #0xff00
            lsr x0, x0, #5
            orr  x0, x0, x3

            lsl x0, x0, #3
            add x1, x1, x0
            add x2, x2, x0

            mov x3, #%d
            mov x4, #0

        loop:
            # Read/Write on DMI socket
            ldr x0, [x1]
            cmp x0, x4
            b.ne fail
            add x0, x0, #1
            str x0, [x1]
            mov x4, x0

            # Read/Write on DMI socket
            ldr x0, [x1]
            cmp x0, x4
            b.ne fail
            add x0, x0, #1
            str x0, [x1]
            mov x4, x0

            # Do DMI invalidation
            str x0, [x2]

            # Read/Write on DMI socket
            ldr x0, [x1]
            cmp x0, x4
            b.ne fail
            add x0, x0, #1
            str x0, [x1]
            mov x4, x0

            # Read/Write on DMI socket
            ldr x0, [x1]
            cmp x0, x4
            b.ne fail
            add x0, x0, #1
            str x0, [x1]
            mov x4, x0

            cmp x0, x3
            b.lt loop

        end:
            wfi
            b end

        fail:
            mov x0, -1
            str x0, [x2]
            b end
    )";

private:
    /*
     * Decrease the number of write per CPU when the number of CPUs increases
     * so that the test does not last forever.
     */
    int m_num_write_per_cpu;

public:
    SC_HAS_PROCESS(CpuArmCortexA53DmiConcurrentInvalTest);

    CpuArmCortexA53DmiConcurrentInvalTest(const sc_core::sc_module_name &n)
        : CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmi>(n)
    {
        char buf[2048];

        m_num_write_per_cpu = NUM_WRITES / p_num_cpu;

        std::snprintf(buf, sizeof(buf), FIRMWARE,
                      CpuTesterDmi::MMIO_ADDR,
                      CpuTesterDmi::DMI_ADDR,
                      m_num_write_per_cpu);
        set_firmware(buf);
    }

    virtual ~CpuArmCortexA53DmiConcurrentInvalTest()
    {
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        if (id != CpuTesterDmi::SOCKET_MMIO) {
            SCP_INFO(SCMOD) << "CPU " << cpuid << "DMI write data: " << std::hex << data << ", len: " << len;

            /*
             * The tester is about to write the new value to memory. Check the
             * old value just in case.
             */
            TEST_ASSERT(m_tester.get_buf_value(cpuid) == data - 1);

            return;
        }

        SCP_INFO(SCMOD) << "CPU write at 0x" << std::hex << addr << ", data: " << std::hex << data <<", len: " << len;

        TEST_ASSERT(data != -1);

        m_tester.dmi_invalidate();
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override
    {
        int cpuid = addr >> 3;

        /* No read on the control socket */
        TEST_ASSERT(id == CpuTesterDmi::SOCKET_DMI);

        SCP_INFO(SCMOD) << "CPU " <<cpuid<< "DMI read data: "<< std::hex << m_tester.get_buf_value(cpuid) << ", len: " << len;

        /* The return value is ignored by the tester */
        return 0;
    }

    virtual bool dmi_request(int id, uint64_t addr, size_t len, tlm::tlm_dmi &ret) override
    {
        SCP_INFO(SCMOD) << "CPU DMI request at 0x" << std::hex << addr << ", len: " << len;

        return true;
    }

    virtual void end_of_simulation() override
    {
        CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmi>::end_of_simulation();

        for (int i = 0; i < p_num_cpu; i++) {
            TEST_ASSERT(m_tester.get_buf_value(i) == m_num_write_per_cpu);
        }
    }
};

constexpr const char * CpuArmCortexA53DmiConcurrentInvalTest::FIRMWARE;

int sc_main(int argc, char *argv[])
{
    return run_testbench<CpuArmCortexA53DmiConcurrentInvalTest>(argc, argv);
}
