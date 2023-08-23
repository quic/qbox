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
#include "test/tester/exclusive.h"

#include "libqbox/components/cpu/arm/cortex-a53.h"
#include "libqbox/qemu-instance.h"

/*
 * Arm Cortex-A53 Load/Store exclusive pair failure.
 *
 * Each CPU tries to lock a memory region which has been pre-locked at the
 * beginning of the test. The corresponding stxr should fail in all cases.
 */
class CpuArmCortexA53LdStExclTest : public CpuTestBench<QemuCpuArmCortexA53, CpuTesterExclusive>
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
        : CpuTestBench<QemuCpuArmCortexA53, CpuTesterExclusive>(n)
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
        CpuTestBench<QemuCpuArmCortexA53, CpuTesterExclusive>::end_of_simulation();
    }
};

constexpr const char* CpuArmCortexA53LdStExclTest::FIRMWARE;

int sc_main(int argc, char* argv[]) { return run_testbench<CpuArmCortexA53LdStExclTest>(argc, argv); }
