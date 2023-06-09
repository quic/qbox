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
#include <scp/report.h>
#include <cci/utils/broker.h>
#include <greensocs/libgsutils.h>
#include <greensocs/gsutils/cciutils.h>
#include "test/cpu.h"
#include "test/tester/mmio.h"

#include "libqbox/components/cpu/arm/cortex-a53.h"
#include "libqbox/qemu-instance.h"

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
class CpuArmCortexA53WriteReadTest : public CpuTestBench<QemuCpuArmCortexA53, CpuTesterMmio>
{
public:
    static constexpr uint64_t NUM_WRITES = 10*1024;

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
    CpuArmCortexA53WriteReadTest(const sc_core::sc_module_name& n)
        : CpuTestBench<QemuCpuArmCortexA53, CpuTesterMmio>(n) {
        char buf[1024];

        std::snprintf(buf, sizeof(buf), FIRMWARE, CpuTesterMmio::MMIO_ADDR, CpuTestBench::BULKMEM_ADDR, NUM_WRITES / p_num_cpu);
        set_firmware(buf);
    }

    virtual ~CpuArmCortexA53WriteReadTest() {}

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override {
        int cpuid = addr >> 3;

        if (data == -1) {
            SCP_WARN(SCMOD) << "An Error was reported by the CPU";
            TEST_ASSERT(false);
        }

        SCP_INFO(SCMOD) << "CPU write at 0x" << std::hex << addr << ", data: " << std::hex << data;

    }

    virtual void end_of_simulation() override {
        CpuTestBench<QemuCpuArmCortexA53, CpuTesterMmio>::end_of_simulation();

    }
};

constexpr const char* CpuArmCortexA53WriteReadTest::FIRMWARE;

int sc_main(int argc, char* argv[]) {
    return run_testbench<CpuArmCortexA53WriteReadTest>(argc, argv);
}
