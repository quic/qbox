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

public:
    SC_HAS_PROCESS(CpuArmCortexA53SimpleHalt);
    CpuArmCortexA53SimpleHalt(const sc_core::sc_module_name &n)
        : CpuTestBench<QemuCpuArmCortexA53, CpuTesterMmio>(n)
        , halt("halt", p_num_cpu)
    {
        char buf[1024];

        map_halt_to_cpus(halt);

        im_halted.resize(p_num_cpu);

        for (int i = 0; i < m_cpus.size(); i++) {
            auto& cpu = m_cpus[i];
            cpu.p_start_powered_off = false;
            cpu.p_start_halted = true;
            im_halted[i] = true;
            }

        std::snprintf(buf, sizeof(buf), FIRMWARE,
                      CpuTesterMmio::MMIO_ADDR,
                      NUM_WRITES*2);
        set_firmware(buf);

        m_writes.resize(p_num_cpu);
        for (int i = 0; i < p_num_cpu; i++) {
            m_writes[i] = 0;
        }

        SC_THREAD(halt_ctrl)

    }

    void halt_ctrl(){
        for (int t = 0; t < 5; t++){
            wait(100000000000, SC_NS);
            sleep(1);
            for (int i = 0; i < m_cpus.size(); i++) {
                if (im_halted[i]){
                    halt[i].write(0);
                    im_halted[i] = false;
                }
            }
        }
    }

    virtual ~CpuArmCortexA53SimpleHalt()
    {
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "CPU write at 0x" << std::hex << addr << ", data: " << std::hex << data;

        TEST_ASSERT(cpuid < p_num_cpu);
        TEST_ASSERT(data == m_writes[cpuid]);

        m_writes[cpuid]++;
        std::cout<<"m_writes (mmio_write) = "<<m_writes[cpuid]<<std::endl;

        TEST_ASSERT(im_halted[cpuid] == false);

        if(m_writes[cpuid] == 5){
            halt[cpuid].write(1);
            im_halted[cpuid] = true;
        }

        if (m_writes[cpuid] == NUM_WRITES){
            halt[cpuid].write(1);
        }
        
    }

    virtual void end_of_simulation() override
    {
        CpuTestBench<QemuCpuArmCortexA53, CpuTesterMmio>::end_of_simulation();
        for (int i = 0; i < p_num_cpu; i++) {
            SCP_INFO(SCMOD) << "m_writes (end_of_simulation) = " << m_writes[i];
            TEST_ASSERT(m_writes[i] == NUM_WRITES);
        }
    }
};

constexpr const char * CpuArmCortexA53SimpleHalt::FIRMWARE;

int sc_main(int argc, char *argv[])
{
    return run_testbench<CpuArmCortexA53SimpleHalt>(argc, argv);
}
