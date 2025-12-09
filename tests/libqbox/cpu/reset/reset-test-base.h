/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <cci_configuration>

#include <cstdio>
#include <vector>

#include <ports/multiinitiator-signal-socket.h>

#include "test/cpu.h"
#include "test/tester/mmio.h"

#include "cortex-a53.h"
#include "qemu-instance.h"
#include "reset_gpio.h"

using namespace sc_core;
using namespace std;
/*
 * Simple ARM Cortex-A53 reset.
 *
 * The CPU starts by fetching its SMP affinity number, then writes ten times to
 * the address (i * 8) of the tester, with i == affinity num, incrementing the
 * value each time. Once it reaches 10, it goes into sleep by calling wfi,
 * effectively starving the kernel and ending the simulation.
 *
 * On each write, the test bench checks the written value. It also checks the
 * number of write at the end of the simulation.
 */
class CpuArmCortexA53SimpleResetBase : public CpuArmTestBench<cpu_arm_cortexA53, CpuTesterMmio>
{
public:
    static constexpr int NUM_WRITES = 1000;

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
            ldr x0, [x1]
            str x1, [x1]
        loop:
            cmp x0, #%d
            b.ge end

            str x0, [x1]
            add x0, x0, #1
            b loop

        end:
            wfi
            b end
    )";

protected:
    std::vector<int> m_writes;
    MultiInitiatorSignalSocket<bool> reset;
    int finished = 0;
    int resets = 0;
    int this_resets = 0;
    int reset_reqs = 0;
    gs::async_event reset_ev;
    sc_core::sc_event done_ev;
    sc_core::sc_event resets_done_ev;
    std::thread m_thread;
#ifdef SYSTEMMODE
    reset_gpio reset_controller_a;
    reset_gpio reset_controller_b;
#endif

public:
    CpuArmCortexA53SimpleResetBase(const sc_core::sc_module_name& n)
        : CpuArmTestBench<cpu_arm_cortexA53, CpuTesterMmio>(n)
#ifdef SYSTEMMODE
        , reset_controller_a("reset_a", m_inst_a)
        , reset_controller_b("reset_b", m_inst_b)
#endif
    {
        char buf[1024];

        for (int i = 0; i < m_cpus.size(); i++) {
            auto& cpu = m_cpus[i];
            cpu.p_start_powered_off = false;
#ifndef SYSTEMMODE
            reset.bind(cpu.reset);
#endif
        }
#ifdef SYSTEMMODE
        reset.bind(reset_controller_a.reset_in);
        reset.bind(reset_controller_b.reset_in);
#endif

        std::snprintf(buf, sizeof(buf), FIRMWARE, CpuTesterMmio::MMIO_ADDR, NUM_WRITES);
        set_firmware(buf);

        m_writes.resize(p_num_cpu);
        for (int i = 0; i < p_num_cpu; i++) {
            m_writes[i] = 0;
        }

        SC_METHOD(reset_method);
        sensitive << reset_ev;
        dont_initialize();

        SC_THREAD(done_method);
        sensitive << done_ev;
    }
    virtual void start_of_simulation() override
    {
        m_thread = std::thread([&]() { reset_pthread(); });
    }
    std::atomic<bool> resetting = false;
    void reset_pthread()
    {
        while (finished < p_num_cpu) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25 * p_num_cpu));
            if (!resetting && finished < p_num_cpu) {
                SCP_INFO(SCMOD) << "Async Reset all CPU's";
                resetting = true;
                reset_ev.notify();
            }
        }
    }
    void reset_method()
    {
        if (this_resets >= p_num_cpu) {
            // Dont test for overlapping resets !!
            this_resets = 0;
            reset_reqs++;
            SCP_INFO(SCMOD) << "Reset all CPU's";
            reset.async_write_vector({ 1, 0 });
        }
    }
    virtual ~CpuArmCortexA53SimpleResetBase() {}

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override
    {
        int cpuid = addr >> 3;
        return m_writes[cpuid];
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        int cpuid = addr >> 3;

        if (data >= 0x80000000ULL) {
            this_resets++;
            TEST_ASSERT(this_resets <= p_num_cpu * 10); // We get double hits sometimes.

            SCP_INFO(SCMOD)("Done RESET CPU {} (Reset {}/{}) ", cpuid, this_resets, (int)p_num_cpu);
            resets++;
            if (this_resets == p_num_cpu) {
                resetting = false;
                resets_done_ev.notify();
            }
            return;
        }
        SCP_INFO(SCMOD) << "CPU " << cpuid << " write " << m_writes[cpuid] + 1 << " at 0x" << std::hex << addr
                        << ", data: 0x" << std::hex << data;

        TEST_ASSERT(cpuid < p_num_cpu);
        TEST_ASSERT(data == m_writes[cpuid]);

        m_writes[cpuid]++;

        TEST_ASSERT(m_writes[cpuid] <= NUM_WRITES);

        if (m_writes[cpuid] == 5) {
            SCP_INFO(SCMOD) << "trigger reset from CPU " << cpuid;
            reset_ev.notify();
            wait(SC_ZERO_TIME); /* in Coroutine mode, we need to explicitly
                                 * call wait to allow SystemC to process the event
                                 * (Otherwise it wont happen till the next end_of_loop)
                                 * NB in co-routine mode it's critical that we dont wait for time to pass
                                 * during reset as this could cause one CPU to block, and hence block the
                                 * whole chain */
        }

        if (m_writes[cpuid] == NUM_WRITES) {
            SCP_INFO(SCMOD) << "CPU " << cpuid << " finished (waiting for " << m_cpus.size() - finished << ")";
            finished++;
        }

        if (finished >= p_num_cpu) {
            SCP_INFO(SCMOD)("All Done");
            done_ev.notify(1, sc_core::SC_SEC);
        }
    }

    void done_method()
    {
        wait(done_ev);
        TEST_ASSERT(this_resets >= p_num_cpu);

        SCP_INFO(SCMOD)("Processing done");
        reset_ev.notify();
        wait(resets_done_ev);
        SCP_INFO(SCMOD)("Resetting done\n");

        reset_ev.async_detach_suspending();
        SCP_INFO(SCMOD) << "Done !";
        sc_stop();
    }

    virtual void end_of_simulation() override
    {
        reset_ev.async_detach_suspending();
        finished = p_num_cpu;
        m_thread.join();
        CpuTestBench<cpu_arm_cortexA53, CpuTesterMmio>::end_of_simulation();
        for (int i = 0; i < p_num_cpu; i++) {
            SCP_INFO(SCMOD) << "m_writes (end_of_simulation) = " << m_writes[i] << " expected " << NUM_WRITES;
            TEST_ASSERT(m_writes[i] >= NUM_WRITES);
        }

        SCP_WARN(SCMOD)("Resets {} Reset requests {}", resets, reset_reqs);
        TEST_ASSERT(reset_reqs >= 1);
        TEST_ASSERT(resets >= (reset_reqs + 1) * p_num_cpu);
        // We will see double resets sometimes due to the 2 reset mechanisms in QEMU.
    }
};
