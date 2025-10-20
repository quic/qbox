/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include <cstdio>
#include <vector>
#include <deque>

#include "async_event.h"
#include "test/cpu.h"
#include "test/tester/mmio.h"
#include <libqemu-cxx/libqemu-cxx.h>

#include "hexagon.h"
#include "qemu-instance.h"
#include "reset_gpio.h"
#include "hexagon_globalreg.h"

/*
 * Hexagon RESET test.
 *  This test will fail if the reset-sequence doesn't flush the translation
 *  blocks, hexagon_cpu_reset_hold must call tb_flush.
 *
 * Debugging:
 *  -p 'test-bench.inst_a.qemu_args="-d in_asm -monitor -S -s tcp:127.0.0.1:5000,server,nowait"'
 *
 */
class CpuHexagonResetGPIOTest : public CpuTestBench<qemu_cpu_hexagon, CpuTesterMmio>
{
    static constexpr int RESET_TRIGGER = 1;
    static constexpr int RESET_DONE = 0;
    /*
     * There is a 100 second general timeout but if we start running and do not
     * finish in less than 20 seconds there is an issue.  This test should
     * finish in less than 100ms.
     */
    static constexpr int TIMEOUT_LIMIT_MS = 20000;

    MultiInitiatorSignalSocket<bool> reset;
    reset_gpio reset_controller;
    hexagon_globalreg hex_gregs;
    std::thread m_thread;
    gs::async_event reset_event;
    int reset_count;
    int reset_done;
    int time_elapsed_ms;

    /*
     * Instructions with .word are ignored or generate an error with Keystone.
     * Use this to get the hex codes interactively
     *     hexagon-llvm-mc --filetype=obj - | hexagon-llvm-objdump -d -
     */
    static constexpr const char* FIRMWARE = R"(
_start:
    r0 = #0x%x
    r1 = #0x%x
    p0 = cmp.eq(r1, #0x%x)
    .word 0x5c00c006  //if (p0) jump reset_done
reset:
    memw(r0) = r1
    .word 0x5800c000 // jump .
reset_done:
    memw(r0) = r1
    .word 0x6440c000 // wait(r0)
    )";

public:
    CpuHexagonResetGPIOTest(const sc_core::sc_module_name& n)
        : CpuTestBench<qemu_cpu_hexagon, CpuTesterMmio>(n)
        , reset_controller("reset", &m_inst_a)
        , hex_gregs("hexagon_globalreg", &m_inst_a)
        , reset_count(0)
        , reset_done(0)
        , time_elapsed_ms(0)
    {
        for (int i = 0; i < m_cpus.size(); i++) {
            auto& cpu = m_cpus[i];
            cpu.p_hexagon_num_threads = m_cpus.size();
            cpu.p_start_powered_off = (i != 0);
        }
        hex_gregs.p_hexagon_start_addr = 0x0;

        reset.bind(reset_controller.reset_in);

        SC_METHOD(reset_method);
        sensitive << reset_event;
        dont_initialize();

        char buf[1024] = { 0 };
        std::snprintf(buf, sizeof(buf), FIRMWARE, static_cast<uint32_t>(CpuTesterMmio::MMIO_ADDR), RESET_TRIGGER,
                      RESET_DONE);
        set_firmware(buf, MEM_ADDR);
    }

    void before_end_of_elaboration() override
    {
        CpuTestBench::before_end_of_elaboration();
        hex_gregs.before_end_of_elaboration();
        qemu::Device hex_gregs_dev = hex_gregs.get_qemu_dev();
        for (int i = 0; i < m_cpus.size(); i++) {
            auto& cpu = m_cpus[i];
            cpu.before_end_of_elaboration();
            qemu::Device cpu_dev = cpu.get_qemu_dev();
            cpu_dev.set_prop_link("global-regs", hex_gregs_dev);
        }
    }

    virtual ~CpuHexagonResetGPIOTest() {}

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        char buf[1024] = { 0 };
        switch (data) {
        case RESET_TRIGGER:
            TEST_ASSERT(reset_count++ == 0);
            /*
             * Load the final firmware image, now the image will write to this address again
             * with a RESET_DONE value and then wait.
             */
            std::snprintf(buf, sizeof(buf), FIRMWARE, static_cast<uint32_t>(CpuTesterMmio::MMIO_ADDR), RESET_DONE,
                          RESET_DONE);
            set_firmware(buf, MEM_ADDR);

            /*
             * If the tb cache isn't flushed during reset we would see this
             * trip more than once.
             */
            m_inst_a.get().tb_invalidate_phys_range(CpuTesterMmio::MMIO_ADDR, CpuTesterMmio::MMIO_ADDR + sizeof(buf));

            set_firmware(buf, MEM_ADDR);
            /*
             * Triggers the reset, this is setup via "sensitive << reset_event" above
             */
            reset_event.notify();
            break;
        case RESET_DONE:
            /*
             * This confirms that we have been reset with the updated firmware image and ran enough
             * code afterward to get here.  This is the beginning of the end of the test.
             */
            reset_done = 1;
            break;
        default:
            TEST_ASSERT(false);
            break;
        }
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override { return 0; }

    void reset_pthread()
    {
        while (1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (reset_done) {
                /*
                 * The detach here allows the system-c to exit.  If we skip
                 * this step sc will just hang.
                 */
                reset_event.async_detach_suspending();
                return;
            }
            TEST_ASSERT(time_elapsed_ms++ < TIMEOUT_LIMIT_MS);
        }
    }
    void reset_method()
    {
        TEST_ASSERT(reset_count == 1);
        reset.async_write_vector({ 1, 0 });
    }

    virtual void start_of_simulation() override
    {
        m_thread = std::thread([&]() { reset_pthread(); });
    }

    virtual void end_of_simulation() override
    {
        m_thread.join();
        TEST_ASSERT(reset_count == 1);
        TEST_ASSERT(time_elapsed_ms < TIMEOUT_LIMIT_MS);
        SCP_INFO(SCMOD) << "reset_count : " << reset_count << "\n";
        SCP_INFO(SCMOD) << "timeout trip : " << time_elapsed_ms << "\n";
        SCP_INFO(SCMOD) << "PASS\n";
        CpuTestBench<qemu_cpu_hexagon, CpuTesterMmio>::end_of_simulation();
    }
};

int sc_main(int argc, char* argv[]) { return run_testbench<CpuHexagonResetGPIOTest>(argc, argv); }
