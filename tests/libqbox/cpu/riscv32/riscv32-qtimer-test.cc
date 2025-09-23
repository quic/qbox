/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include <cstdio>
#include <vector>
#include <deque>
#include <fstream>
#include <iostream>

#include <systemc>
#include <cci_configuration>

#include "async_event.h"
#include "test/cpu.h"
#include "test/tester/mmio.h"
#include <libqemu-cxx/libqemu-cxx.h>

#include "riscv32.h"
#include "qemu-instance.h"
#include "reset_gpio.h"
#include "riscv-aclint-mtimer.h"
#include <ports/multiinitiator-signal-socket.h>

/*
 * RISC-V 32-bit ACLINT Timer IRQ test.
 *
 * This test verifies that timer interrupts can be delivered to the RISC-V CPU
 * through the RISC-V ACLINT machine timer infrastructure.
 *
 * Test design:
 * - Uses RISC-V ACLINT MTimer for timer interrupt delivery
 * - Firmware programs timer for periodic interrupts (every 1ms)
 * - Test verifies that multiple timer interrupts are received
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

class CpuRiscv32TimerIrqTest : public CpuRiscvTestBench<cpu_riscv32, CpuTesterMmio>
{
    static constexpr int TIMER_SETUP_COMPLETE = 1;
    static constexpr int TIMER_INTERRUPT_HANDLED = 2;
    static constexpr int TEST_COMPLETE = 3;
    static constexpr int EXPECTED_TIMER_INTERRUPTS = 5; // Expect at least 5 timer interrupts
    static constexpr int TIMEOUT_LIMIT_MS = 120000;

    // Timer device memory addresses (mapped by router)
    static constexpr uint64_t ACLINT_MTIMER_BASE = 0x2000000; // ACLINT MTimer base
    static constexpr uint64_t ACLINT_MTIMER_SIZE = 0x8000;    // ACLINT MTimer size

    // Timer interrupt period (1ms = 10,000 timer ticks at 10MHz)
    static constexpr uint64_t TIMER_PERIOD_TICKS = 10000;

    MultiInitiatorSignalSocket<bool> reset;
    reset_gpio reset_controller;

    // Timer components
    riscv_aclint_mtimer aclint_mtimer;

    std::thread m_thread;
    gs::async_event reset_event;
    int timer_interrupt_count;
    int test_status;
    int time_elapsed_ms;

    /*
     * Load RISC-V 32-bit timer firmware from binary file compiled with LLVM tools.
     * The binary contains the constants at the end that need to be patched.
     */
    void load_firmware_binary(uint32_t mmio_addr, uint32_t aclint_addr, uint32_t timer_period, uint32_t setup_val,
                              uint32_t handled_val, uint32_t complete_val)
    {
        SCP_DEBUG(SCMOD) << "load_firmware_binary called with mmio_addr=0x" << std::hex << mmio_addr
                         << ", aclint_addr=0x" << aclint_addr << ", timer_period=" << std::dec << timer_period
                         << ", setup_val=" << setup_val << ", handled_val=" << handled_val
                         << ", complete_val=" << complete_val;

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

        // Patch the constants in the binary (last 24 bytes: mmio_addr, aclint_addr, timer_period, setup_val,
        // handled_val, complete_val)
        TEST_ASSERT(size >= 24);
        uint32_t* mmio_addr_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 24);
        uint32_t* aclint_addr_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 20);
        uint32_t* timer_period_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 16);
        uint32_t* setup_val_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 12);
        uint32_t* handled_val_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 8);
        uint32_t* complete_val_ptr = reinterpret_cast<uint32_t*>(firmware_data.data() + size - 4);

        *mmio_addr_ptr = mmio_addr;
        *aclint_addr_ptr = aclint_addr;
        *timer_period_ptr = timer_period;
        *setup_val_ptr = setup_val;
        *handled_val_ptr = handled_val;
        *complete_val_ptr = complete_val;

        SCP_DEBUG(SCMOD) << "Patched firmware constants:";
        SCP_DEBUG(SCMOD) << "  mmio_addr = 0x" << std::hex << mmio_addr;
        SCP_DEBUG(SCMOD) << "  aclint_addr = 0x" << aclint_addr;
        SCP_DEBUG(SCMOD) << "  timer_period = " << std::dec << timer_period;
        SCP_DEBUG(SCMOD) << "  setup_val = " << setup_val;
        SCP_DEBUG(SCMOD) << "  handled_val = " << handled_val;
        SCP_DEBUG(SCMOD) << "  complete_val = " << complete_val;

        SCP_INFO(SCMOD) << "Loading RISC-V Timer IRQ test firmware from " << firmware_path;

        // Load firmware directly into memory
        SCP_DEBUG(SCMOD) << "Loading RISC-V firmware at 0x" << std::hex << MEM_ADDR << " (MEM_ADDR), size=" << std::dec
                         << size << " bytes";
        m_mem.load.ptr_load(firmware_data.data(), MEM_ADDR, size);

        SCP_DEBUG(SCMOD) << "Firmware loaded successfully";
    }

public:
    CpuRiscv32TimerIrqTest(const sc_core::sc_module_name& n)
        : CpuRiscvTestBench<cpu_riscv32, CpuTesterMmio>(n)
        , reset_controller("reset", &m_inst_a)
        , aclint_mtimer("aclint_mtimer", &m_inst_a)
        , timer_interrupt_count(0)
        , test_status(0)
        , time_elapsed_ms(0)
    {
        // Debug: Check parameter values during construction
        SCP_INFO(SCMOD) << "Constructor: p_num_cpu=" << (int)p_num_cpu << ", m_cpus.size()=" << m_cpus.size();

        // Configure timer parameters immediately after construction
        aclint_mtimer.p_num_harts = p_num_cpu;
        aclint_mtimer.p_hartid_base = 0;
        aclint_mtimer.p_timecmp_base = 0x0;
        aclint_mtimer.p_time_base = 0x7ff8;
        aclint_mtimer.p_aperture_size = ACLINT_MTIMER_SIZE;
        aclint_mtimer.p_timebase_freq = 10000000;
        aclint_mtimer.p_provide_rdtime = true;

        SCP_INFO(SCMOD) << "Constructor: After setting timer params, aclint_mtimer.p_num_harts="
                        << (unsigned int)aclint_mtimer.p_num_harts;

        // Map timer device to memory
        m_router.add_target(aclint_mtimer.socket, ACLINT_MTIMER_BASE, ACLINT_MTIMER_SIZE);

        SCP_INFO(SCMOD) << "Timer device mapping:";
        SCP_INFO(SCMOD) << "  ACLINT MTimer at 0x" << std::hex << ACLINT_MTIMER_BASE << " (size 0x"
                        << ACLINT_MTIMER_SIZE << ")";

        for (int i = 0; i < m_cpus.size(); i++) {
            auto& cpu = m_cpus[i];

            // Connect reset to CPU (needed for CPU to start execution)
            reset.bind(cpu.reset);
            SCP_DEBUG(SCMOD) << "Connected reset to cpu[" << i << "].reset";
        }

        SC_METHOD(reset_method);
        sensitive << reset_event;
        dont_initialize();

        // Load RISC-V 32-bit timer firmware compiled with LLVM tools
        load_firmware_binary(static_cast<uint32_t>(CpuTesterMmio::MMIO_ADDR), // MMIO communication address
                             static_cast<uint32_t>(ACLINT_MTIMER_BASE),       // ACLINT MTimer base address
                             static_cast<uint32_t>(TIMER_PERIOD_TICKS),       // Timer period in ticks
                             TIMER_SETUP_COMPLETE,                            // Timer setup complete value
                             TIMER_INTERRUPT_HANDLED,                         // Timer interrupt handled value
                             TEST_COMPLETE                                    // Test complete value
        );

        // Trigger initial reset to start CPU execution
        reset_event.notify(sc_core::sc_time(1, sc_core::SC_US));
    }

    virtual ~CpuRiscv32TimerIrqTest() {}

    void before_end_of_elaboration() override
    {
        // Force re-set the timer parameters before elaboration
        aclint_mtimer.p_num_harts = p_num_cpu;
        SCP_INFO(SCMOD) << "Before elaboration: p_num_cpu=" << (int)p_num_cpu
                        << ", aclint_mtimer.p_num_harts=" << (unsigned int)aclint_mtimer.p_num_harts;

        // Call parent before_end_of_elaboration - this will trigger timer component elaboration
        CpuRiscvTestBench<cpu_riscv32, CpuTesterMmio>::before_end_of_elaboration();

        // Check if timer component properly initialized the vector
        SCP_INFO(SCMOD) << "After parent elaboration - Timer IRQ binding: m_cpus.size()=" << m_cpus.size()
                        << ", p_num_cpu=" << (int)p_num_cpu << ", timer_irq.size()=" << aclint_mtimer.timer_irq.size();

        // Force timer component to initialize its vector early by calling its timer initialization method
        aclint_mtimer.timer_irq.init(p_num_cpu,
                                     [](const char* n, size_t i) { return new QemuInitiatorSignalSocket(n); });
        SCP_INFO(SCMOD) << "Forced timer_irq init, size=" << aclint_mtimer.timer_irq.size();

        // Connect timer IRQ outputs to CPU timer interrupt inputs (IRQ pin 7)
        // In SiFive E architecture, ACLINT MTimer connects directly to CPU timer interrupt pin
        for (int i = 0; i < m_cpus.size() && i < aclint_mtimer.timer_irq.size(); i++) {
            auto& cpu = m_cpus[i];
            aclint_mtimer.timer_irq[i].bind(cpu.irq_in[7]);
            SCP_INFO(SCMOD) << "Connected ACLINT MTimer timer_irq[" << i << "] to cpu[" << i
                            << "].irq_in[7] (machine timer interrupt)";
        }
    }

    void end_of_elaboration() override
    {
        // Call parent end_of_elaboration - this will realize the QEMU devices
        CpuRiscvTestBench<cpu_riscv32, CpuTesterMmio>::end_of_elaboration();

        SCP_INFO(SCMOD) << "End of elaboration - timer_irq.size()=" << aclint_mtimer.timer_irq.size();
        SCP_INFO(SCMOD) << "End of elaboration - RISC-V ACLINT MTimer -> CPU timer interrupt connections established";
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        SCP_INFO(SCMOD) << "MMIO write: addr=0x" << std::hex << addr << " data=0x" << data
                        << " (expect 1=timer_setup, 2=timer_irq_handled, 3=test_complete), timer_irq_count=" << std::dec
                        << timer_interrupt_count;

        switch (data) {
        case TIMER_SETUP_COMPLETE:
            // Firmware has completed timer setup
            SCP_INFO(SCMOD) << "Timer setup completed by firmware, periodic timer interrupts should now be active";
            test_status = TIMER_SETUP_COMPLETE;
            break;

        case TIMER_INTERRUPT_HANDLED:
            // Firmware has handled a timer interrupt
            timer_interrupt_count++;
            SCP_INFO(SCMOD) << "Timer interrupt #" << timer_interrupt_count << " handled by firmware";
            test_status = TIMER_INTERRUPT_HANDLED;

            // Check if we've received enough interrupts to complete the test
            if (timer_interrupt_count >= EXPECTED_TIMER_INTERRUPTS) {
                SCP_INFO(SCMOD) << "Received " << timer_interrupt_count << " timer interrupts, test successful!";
                test_status = TEST_COMPLETE;
                sc_core::sc_stop();
            }
            break;

        case TEST_COMPLETE:
            // Test completed successfully (alternative completion path)
            SCP_INFO(SCMOD) << "Timer IRQ test completed successfully, stopping simulation";
            test_status = TEST_COMPLETE;
            sc_core::sc_stop();
            break;

        default:
            SCP_INFO(SCMOD) << "Unexpected MMIO write value: " << data << " (might be normal during startup)";
            break;
        }
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override { return 0; }

    void timer_pthread()
    {
        while (1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (test_status == TEST_COMPLETE) {
                /*
                 * The detach here allows the system-c to exit.
                 */
                reset_event.async_detach_suspending();
                return;
            }

            // Add debug output every second to track progress
            if (time_elapsed_ms % 1000 == 0) {
                SCP_INFO(SCMOD) << "Waiting for RISC-V Timer IRQ test completion... time: " << time_elapsed_ms
                                << "ms, timer_interrupts=" << timer_interrupt_count;
            }

            TEST_ASSERT(time_elapsed_ms++ < TIMEOUT_LIMIT_MS);
        }
    }

    void reset_method()
    {
        SCP_DEBUG(SCMOD) << "Triggering initial reset to start CPU execution";

        // Send reset pulse: assert then immediately deassert
        reset.async_write_vector({ true, false });

        SCP_DEBUG(SCMOD) << "Reset pulse completed - CPU should start executing";
    }

    virtual void start_of_simulation() override
    {
        m_thread = std::thread([&]() { timer_pthread(); });
    }

    virtual void end_of_simulation() override
    {
        m_thread.join();
        TEST_ASSERT(timer_interrupt_count >= EXPECTED_TIMER_INTERRUPTS);
        TEST_ASSERT(test_status == TEST_COMPLETE);
        TEST_ASSERT(time_elapsed_ms < TIMEOUT_LIMIT_MS);
        SCP_INFO(SCMOD) << "timer_interrupt_count : " << timer_interrupt_count;
        SCP_INFO(SCMOD) << "test_status : " << test_status;
        SCP_INFO(SCMOD) << "timeout trip : " << time_elapsed_ms;
        SCP_INFO(SCMOD) << "PASS";
        CpuRiscvTestBench<cpu_riscv32, CpuTesterMmio>::end_of_simulation();
    }
};

int sc_main(int argc, char* argv[]) { return run_testbench<CpuRiscv32TimerIrqTest>(argc, argv); }
