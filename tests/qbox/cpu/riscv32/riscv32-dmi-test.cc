/*
 * This file is part of libqbox
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <cstdio>
#include <vector>
#include <deque>
#include <fstream>
#include <iostream>

#include "test/cpu.h"
#include "test/tester/dmi.h"

#include "riscv32.h"
#include "qemu-instance.h"

/*
 * RISC-V32 DMI test.
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
template <class CPU, class TESTER>
class CpuRiscvTestBench : public CpuTestBench<CPU, TESTER>
{
public:
    CpuRiscvTestBench(const sc_core::sc_module_name& n): CpuTestBench<CPU, TESTER>(n)
    {
        int i = 0;
        for (CPU& cpu : CpuTestBench<CPU, TESTER>::m_cpus) {
            cpu.p_hartid = i++;
        }
    }
};

class CpuRiscv32DmiTest : public CpuRiscvTestBench<cpu_riscv32, CpuTesterDmi>
{
public:
    static constexpr int NUM_WRITES = 512;

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
    std::vector<int> m_dmi_ok;
    bool m_simulation_stopped;

    bool last_access_was_io(int cpuid)
    {
        TEST_ASSERT(cpuid < p_num_cpu);
        return m_last_access_io[cpuid];
    }

    void enable_dmi(int cpuid) { m_dmi_enabled[cpuid] = true; }

    void disable_dmi(int cpuid) { m_dmi_enabled[cpuid] = false; }

    /*
     * Load RISC-V 32-bit firmware from binary file compiled with LLVM tools.
     * The binary contains the constants at the end that need to be patched.
     */
    void load_firmware_binary(uint32_t dmi_addr, uint32_t mmio_addr, uint32_t num_writes)
    {
#ifdef FIRMWARE_BIN_PATH
        // Load the compiled firmware binary
        const char* firmware_path = FIRMWARE_BIN_PATH;
        std::ifstream file(firmware_path, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            SCP_FATAL(SCMOD) << "Failed to open RISC-V firmware file: " << firmware_path;
            TEST_ASSERT(false);
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> firmware_data(size);
        if (!file.read(reinterpret_cast<char*>(firmware_data.data()), size)) {
            SCP_FATAL(SCMOD) << "Failed to read RISC-V firmware file: " << firmware_path;
            TEST_ASSERT(false);
        }

        // Patch the constants in the binary (last 12 bytes: dmi_addr, mmio_addr, num_writes)
        if (size >= 12) {
            uint32_t* dmi_addr_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 12);
            uint32_t* mmio_addr_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 8);
            uint32_t* num_writes_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 4);

            SCP_INFO(SCMOD) << "Before patching: dmi=0x" << std::hex << *dmi_addr_ptr << ", mmio=0x" << *mmio_addr_ptr
                            << ", num_writes=0x" << *num_writes_ptr;

            *dmi_addr_ptr = dmi_addr;
            *mmio_addr_ptr = mmio_addr;
            *num_writes_ptr = num_writes;

            SCP_INFO(SCMOD) << "After patching: dmi=0x" << std::hex << *dmi_addr_ptr << ", mmio=0x" << *mmio_addr_ptr
                            << ", num_writes=0x" << *num_writes_ptr;
            SCP_INFO(SCMOD) << "Constants will be at addresses: dmi=0x" << std::hex << (MEM_ADDR + size - 12)
                            << ", mmio=0x" << (MEM_ADDR + size - 8) << ", num_writes=0x" << (MEM_ADDR + size - 4);
        }

        // Load firmware directly into memory
        SCP_INFO(SCMOD) << "Loading RISC-V DMI test firmware at 0x" << std::hex << MEM_ADDR << ", size=" << std::dec
                        << size << " bytes";
        m_mem.load.ptr_load(firmware_data.data(), MEM_ADDR, size);
#else
        SCP_FATAL(SCMOD) << "FIRMWARE_BIN_PATH not defined - LLVM firmware compilation failed";
        TEST_ASSERT(false);
#endif
    }

public:
    CpuRiscv32DmiTest(const sc_core::sc_module_name& n): CpuRiscvTestBench<cpu_riscv32, CpuTesterDmi>(n)
    {
        SCP_DEBUG(SCMOD) << "CpuRiscv32DmiTest constructor";
        m_num_write_per_cpu = NUM_WRITES / p_num_cpu;

        // Load RISC-V 32-bit binary firmware compiled with LLVM tools
        load_firmware_binary(static_cast<uint32_t>(CpuTesterDmi::DMI_ADDR),
                             static_cast<uint32_t>(CpuTesterDmi::MMIO_ADDR), m_num_write_per_cpu);

        m_last_access_io.resize(p_num_cpu);
        std::fill(m_last_access_io.begin(), m_last_access_io.end(), false);

        m_state.resize(p_num_cpu);
        std::fill(m_state.begin(), m_state.end(), ST_START);

        m_dmi_enabled.resize(p_num_cpu);
        std::fill(m_dmi_enabled.begin(), m_dmi_enabled.end(), true);

        m_dmi_ok.resize(p_num_cpu);
        std::fill(m_dmi_ok.begin(), m_dmi_ok.end(), 0);

        m_simulation_stopped = false;
    }

    virtual ~CpuRiscv32DmiTest() {}

    void ctrl_write(uint64_t addr, uint64_t data, size_t len)
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "CPU " << (addr >> 3) << " write at 0x" << std::hex << addr << ", data: " << std::hex << data
                        << ", len: " << len;

        if (m_dmi_ok[cpuid]) {
            if (m_dmi_ok[cpuid] > 1) {
                TEST_ASSERT(!last_access_was_io(cpuid));
            }
            m_dmi_ok[cpuid]++;
        }

        switch (m_state[cpuid]) {
        case ST_START:
            TEST_ASSERT(last_access_was_io(cpuid));
            break;

        case ST_READ_DMI:
            break;

        case ST_WRITE_DMI:
            m_tester.dmi_invalidate(addr, addr + len - 1);
            m_dmi_ok[cpuid] = false;
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

        m_last_access_io[cpuid] = false;

        // Check if all CPUs have completed their expected number of writes
        // Only check after this CPU has completed to avoid premature termination
        if (!m_simulation_stopped && m_tester.get_buf_value(cpuid) >= m_num_write_per_cpu) {
            SCP_INFO(SCMOD) << "CPU " << cpuid << " completed " << m_tester.get_buf_value(cpuid)
                            << " writes, checking if all CPUs done";

            bool all_completed = true;
            for (int i = 0; i < p_num_cpu; i++) {
                SCP_INFO(SCMOD) << "CPU " << i << " has " << m_tester.get_buf_value(i) << " writes (need "
                                << m_num_write_per_cpu << ")";
                if (m_tester.get_buf_value(i) < m_num_write_per_cpu) {
                    all_completed = false;
                    break;
                }
            }

            if (all_completed) {
                m_simulation_stopped = true;
                SCP_INFO(SCMOD) << "All CPUs completed their work, stopping simulation";
                sc_core::sc_stop();
            } else {
                SCP_INFO(SCMOD) << "Not all CPUs completed yet, continuing...";
            }
        }
    }

    void dmi_access(uint64_t addr)
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "CPU " << (addr >> 3) << " DMI accessed at 0x" << std::hex << addr;

        TEST_ASSERT(m_dmi_ok[cpuid] == false || m_last_access_io[cpuid] == false);
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

    virtual void dmi_request_failed(int id, uint64_t addr, size_t len, tlm::tlm_dmi& ret) override
    {
        SCP_INFO(SCMOD) << "CPU " << (addr >> 3) << " DMI request failed 0x" << std::hex << addr << ", len: " << len;
        m_dmi_ok[addr >> 3] = 0;
    }

    virtual bool dmi_request(int id, uint64_t addr, size_t len, tlm::tlm_dmi& ret) override
    {
        int cpuid = addr >> 3;

        SCP_INFO(SCMOD) << "CPU " << (addr >> 3) << " DMI request at 0x" << std::hex << addr << ", len: " << len;

        /*
         * Restrict the DMI region to the 8 bytes this CPU writes. So when it
         * will invalidate it, the other CPUs won't be impacted.
         */
        ret.set_start_address(addr);
        ret.set_end_address(addr + len - 1);
        m_dmi_ok[cpuid] = m_dmi_enabled[cpuid] ? 1 : 0;
        return m_dmi_enabled[cpuid];
    }

    virtual void end_of_simulation() override
    {
        CpuTestBench<cpu_riscv32, CpuTesterDmi>::end_of_simulation();

        for (int i = 0; i < p_num_cpu; i++) {
            SCP_INFO(SCMOD) << "CPU " << i << " completed " << m_tester.get_buf_value(i) << " writes, expecting "
                            << m_num_write_per_cpu;
            if (m_tester.get_buf_value(i) != m_num_write_per_cpu) {
                SCP_FATAL(SCMOD) << "CPU " << i << " failed: got " << m_tester.get_buf_value(i) << ", expected "
                                 << m_num_write_per_cpu;
            }
            TEST_ASSERT(m_tester.get_buf_value(i) == m_num_write_per_cpu);
        }
    }
};

int sc_main(int argc, char* argv[]) { return run_testbench<CpuRiscv32DmiTest>(argc, argv); }
