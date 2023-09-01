/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs SAS
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdio>
#include <vector>

#include "test/cpu.h"
#include "test/tester/mmio.h"

#include "libqbox/components/cpu/arm/cortex-a53.h"
#include "libqbox/qemu-instance.h"

using namespace sc_core;
using namespace std;
/*
 * Simple ARM Cortex-A53 halt.
 *
 * The CPU starts by fetching its SMP affinity number, then writes ten times to
 * the address (i * 8) of the tester, with i == affinity num, incrementing the
 * value each time. Once it reaches 10, it goes into sleep by calling wfi,
 * effectively starving the kernel and ending the simulation.
 *
 * On each write, the test bench checks the written value. It also checks the
 * number of write at the end of the simulation.
 */
class CpuArmCortexA53SimpleHalt : public CpuTestBench<QemuCpuArmCortexA53, CpuTesterMmio>
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
    std::vector<bool> im_halted;
    sc_core::sc_vector<sc_core::sc_out<bool>> halt;
    int finished = 0;

public:
    SC_HAS_PROCESS(CpuArmCortexA53SimpleHalt);
    CpuArmCortexA53SimpleHalt(const sc_core::sc_module_name& n)
        : CpuTestBench<QemuCpuArmCortexA53, CpuTesterMmio>(n), halt("halt", p_num_cpu)
    {
        char buf[1024];

        map_halt_to_cpus(halt);

        im_halted.resize(p_num_cpu);

        for (int i = 0; i < m_cpus.size(); i++) {
            auto& cpu = m_cpus[i];
            cpu.p_start_powered_off = true;
            im_halted[i] = true;
        }

        std::snprintf(buf, sizeof(buf), FIRMWARE, CpuTesterMmio::MMIO_ADDR, NUM_WRITES * 2);
        set_firmware(buf);

        m_writes.resize(p_num_cpu);
        for (int i = 0; i < p_num_cpu; i++) {
            m_writes[i] = 0;
        }

        SC_THREAD(halt_ctrl);
    }

    void halt_ctrl()
    {
        // Mark this as unsuspendable, since QEMU may 'suspend' all activity if we mark all cores as halted.
        sc_core::sc_unsuspendable();
        halt[0].write(1); // make sure we start halted

        while (finished < m_cpus.size()) {
            wait(100, SC_MS);
            for (int i = 0; i < m_cpus.size(); i++) {
                if ((im_halted[i] || m_writes[i] == 5) && m_writes[i] < NUM_WRITES) {
                    SCP_INFO(SCMOD) << "release CPU " << i;
                    halt[i].write(0);
                    im_halted[i] = false;
                }
            }
        }
        wait(100, SC_MS);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // the halt below will kick the CPU, causing there to be an async run on cpu (kick) to
        // be pending when we effectively shut down the simulation. This gives a little time to be sure the kick is
        // processed.

        sc_core::sc_suspendable();
    }

    virtual ~CpuArmCortexA53SimpleHalt() {}

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "CPU " << cpuid << " write " << m_writes[cpuid] + 1 << " at 0x" << std::hex << addr
                        << ", data: " << std::hex << data;

        TEST_ASSERT(im_halted[cpuid] == false);
        TEST_ASSERT(cpuid < p_num_cpu);
        TEST_ASSERT(data == m_writes[cpuid]);

        m_writes[cpuid]++;

        TEST_ASSERT(m_writes[cpuid] <= NUM_WRITES);

        if (m_writes[cpuid] == 5) {
            SCP_INFO(SCMOD) << "halt CPU " << cpuid;
            halt[cpuid].write(1);
            im_halted[cpuid] = true;
        }

        if (m_writes[cpuid] == NUM_WRITES) {
            SCP_INFO(SCMOD) << "CPU " << cpuid << " finished (waiting for " << m_cpus.size() - finished << ")";
            finished++;
            halt[cpuid].write(1);
            im_halted[cpuid] = true;
        }
    }

    virtual void end_of_simulation() override
    {
        CpuTestBench<QemuCpuArmCortexA53, CpuTesterMmio>::end_of_simulation();
        for (int i = 0; i < p_num_cpu; i++) {
            SCP_INFO(SCMOD) << "m_writes (end_of_simulation) = " << m_writes[i] << " expected " << NUM_WRITES;
            TEST_ASSERT(m_writes[i] == NUM_WRITES);
        }
    }
};

constexpr const char* CpuArmCortexA53SimpleHalt::FIRMWARE;

int sc_main(int argc, char* argv[]) { return run_testbench<CpuArmCortexA53SimpleHalt>(argc, argv); }
