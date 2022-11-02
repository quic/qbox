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
 * ARM Cortex-A53 DMI test.
 *
 * In this test, each CPU has its own DMI region it can access and invalidate.
 * Thus, a CPU invalidating its region does not affect other CPUs. The test
 * embeds a small finite state machine. Each CPU access in its DMI region is
 * followed by a write to the control socket where the test can perform some
 * checks and move from one state to the next. Every two accesses to the DMI
 * region (a read then a write), the test invalidates the DMI region of the CPU
 * and disable DMI hint. It then checks the following accesses two accesses are
 * performed as I/O access and not DMI one. It then re-enable DMI hint and the
 * test repeats.
 */
class CpuArmCortexA53DmiTest : public CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmi>
{
public:
    static constexpr int NUM_WRITES = 512;

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

            # Init: will ask for a DMI pointer and go from ST_START to ST_READ_DMI
            ldr x0, [x1]
            str x0, [x2]

        loop:
            # Read on DMI socket, report the read on the control socket
            ldr x0, [x1]
            str x0, [x2]

            add x0, x0, #1

            # Write on DMI socket, report the write on the control socket
            str x0, [x1]
            str x0, [x2]

            cmp x0, #%d
            b.ne loop

        end:
            wfi
            b end
    )";

protected:
    enum State {
        ST_START = -1,
        ST_READ_DMI = 0,
        ST_WRITE_DMI,
        ST_READ_IO,
        ST_WRITE_IO,
    };

    /*
     * Decrease the number of write per CPU when the number of CPUs increases
     * so that the test does not last forever.
     */
    int m_num_write_per_cpu;

    std::vector<State> m_state;
    std::vector<bool> m_last_access_io;
    std::vector<bool> m_dmi_enabled;

    bool last_access_was_io(int cpuid)
    {
        bool ret;

        TEST_ASSERT(cpuid < p_num_cpu);
        ret = m_last_access_io[cpuid];

        m_last_access_io[cpuid] = false;

        return ret;
    }

    void enable_dmi(int cpuid)
    {
        m_dmi_enabled[cpuid] = true;
    }

    void disable_dmi(int cpuid)
    {
        m_dmi_enabled[cpuid] = false;
    }

public:
    SC_HAS_PROCESS(CpuArmCortexA53DmiTest);

    CpuArmCortexA53DmiTest(const sc_core::sc_module_name &n)
        : CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmi>(n)
    {
        char buf[1024];

        m_num_write_per_cpu = NUM_WRITES / p_num_cpu;

        std::snprintf(buf, sizeof(buf), FIRMWARE,
                      CpuTesterDmi::MMIO_ADDR,
                      CpuTesterDmi::DMI_ADDR,
                      m_num_write_per_cpu);
        set_firmware(buf);

        m_last_access_io.resize(p_num_cpu);
        std::fill(m_last_access_io.begin(), m_last_access_io.end(), false);

        m_state.resize(p_num_cpu);
        std::fill(m_state.begin(), m_state.end(), ST_START);

        m_dmi_enabled.resize(p_num_cpu);
        std::fill(m_dmi_enabled.begin(), m_dmi_enabled.end(), true);
    }

    virtual ~CpuArmCortexA53DmiTest()
    {
    }

    void ctrl_write(uint64_t addr, uint64_t data, size_t len)
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "CPU write at 0x" << std::hex << addr << ", data: " << std::hex << data <<", len: " << len;

        switch (m_state[cpuid]) {
        case ST_START:
            TEST_ASSERT(last_access_was_io(cpuid));
            break;

        case ST_READ_DMI:
            TEST_ASSERT(!last_access_was_io(cpuid));
            break;

        case ST_WRITE_DMI:
            TEST_ASSERT(!last_access_was_io(cpuid));
            m_tester.dmi_invalidate(addr, addr + len - 1);
            disable_dmi(cpuid);
            break;

        case ST_READ_IO:
            TEST_ASSERT(last_access_was_io(cpuid));
            enable_dmi(cpuid);
            break;

        case ST_WRITE_IO:
            TEST_ASSERT(last_access_was_io(cpuid));
            break;
        }

        m_state[cpuid] = State((int(m_state[cpuid]) + 1) % 4);
    }

    void dmi_access(uint64_t addr)
    {
        int cpuid = addr >> 3;

        TEST_ASSERT(m_last_access_io[cpuid] == false);
        m_last_access_io[cpuid] = true;
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        switch (id) {
        case CpuTesterDmi::SOCKET_MMIO:
            ctrl_write(addr, data, len);
            break;

        case CpuTesterDmi::SOCKET_DMI:
            dmi_access(addr);
            break;
        }
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override
    {
        /* No read on the control socket */
        TEST_ASSERT(id == CpuTesterDmi::SOCKET_DMI);

        dmi_access(addr);

        /* The return value is ignored by the tester */
        return 0;
    }

    virtual bool dmi_request(int id, uint64_t addr, size_t len, tlm::tlm_dmi &ret) override
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "CPU DMI request at 0x" << std::hex << addr << ", len: " << len;

        /*
         * Restrict the DMI region to the 8 bytes this CPU writes. So when it
         * will invalidate it, the other CPUs won't be impacted.
         */
        ret.set_start_address(addr);
        ret.set_end_address(addr + len - 1);

        return m_dmi_enabled[cpuid];
    }

    virtual void end_of_simulation() override
    {
        CpuTestBench<QemuCpuArmCortexA53, CpuTesterDmi>::end_of_simulation();

        for (int i = 0; i < p_num_cpu; i++) {
            TEST_ASSERT(m_tester.get_buf_value(i) == m_num_write_per_cpu);
        }
    }
};

constexpr const char * CpuArmCortexA53DmiTest::FIRMWARE;

int sc_main(int argc, char *argv[])
{
    return run_testbench<CpuArmCortexA53DmiTest>(argc, argv);
}
