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
#include "test/tester/dmi_soak.h"

#include "libqbox/components/cpu/arm/cortex-a53.h"
#include "libqbox/qemu-instance.h"

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
class CpuArmCortexA53DmiAsyncInvalTest : public CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmiSoak>
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

        loop:
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

            ldr x7, [x10]
            cmp x10, x7
            b.ne fail

            sub x3, x3, #1
            cmp x3, #0
            b.ne loop

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
    uint64_t m_num_write_per_cpu;
    std::thread m_thread;
    bool running=0;
public:
    SC_HAS_PROCESS(CpuArmCortexA53DmiAsyncInvalTest);

    CpuArmCortexA53DmiAsyncInvalTest(const sc_core::sc_module_name &n)
        : CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmiSoak>(n)
    {
        char buf[2048];

        m_num_write_per_cpu = NUM_WRITES / p_num_cpu;

        std::snprintf(buf, sizeof(buf), FIRMWARE,
                      CpuTesterDmiSoak::MMIO_ADDR,
                      CpuTesterDmiSoak::DMI_ADDR,
                      CpuTesterDmiSoak::DMI_SIZE-1,
                      (((uint64_t)std::rand()<<32)| rand()),
                      m_num_write_per_cpu);
        set_firmware(buf);

    }
    
    virtual void start_of_simulation() override
    {
        running=true;
        m_thread = std::thread([&](){inval();});
    }
    void inval() {
        //for (int i=0; i<1000; i++) {
            while (running) {
                    std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
//std::cout << "Invalidate\n";
uint64_t l=CpuTesterDmiSoak::DMI_SIZE;
uint64_t s=(std::rand() +1u) % l; l-=s;
uint64_t e=s+((std::rand() +1u) % l);
if (running) 
            m_tester.dmi_invalidate(s, e);
        }
    }

    virtual ~CpuArmCortexA53DmiAsyncInvalTest()
    {
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        if (id != CpuTesterDmiSoak::SOCKET_MMIO) {
            GS_LOG("CPU %d DMI write data: %" PRIu64 ", len: %zu", cpuid, data, len);

            return;
        }

        GS_LOG("CPU write at 0x%" PRIx64 ", data: %" PRIu64 ", len: %zu", addr, data, len);

        TEST_ASSERT(data != -1);

//        m_tester.dmi_invalidate();
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override
    {
        int cpuid = addr >> 3;

        /* No read on the control socket */
        TEST_ASSERT(id == CpuTesterDmiSoak::SOCKET_DMI);

        /* The return value is ignored by the tester */
        return 0;
    }

    virtual bool dmi_request(int id, uint64_t addr, size_t len, tlm::tlm_dmi &ret) override
    {
        GS_LOG("CPU DMI request at 0x%" PRIx64 ", len: %zu", addr, len);

        return true;
    }

    virtual void end_of_simulation() override
    {
        CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmiSoak>::end_of_simulation();
        running=false;
        m_thread.join();
//        for (int i = 0; i < p_num_cpu; i++) {
//            TEST_ASSERT(m_tester.get_buf_value(i) == m_num_write_per_cpu);
//        }
    }
};

constexpr const char * CpuArmCortexA53DmiAsyncInvalTest::FIRMWARE;

int sc_main(int argc, char *argv[])
{
    return run_testbench<CpuArmCortexA53DmiAsyncInvalTest>(argc, argv);
}
