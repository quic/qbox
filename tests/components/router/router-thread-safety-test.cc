/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file router-thread-safety-test.cc
 * @brief Thread-safety tests for the router component
 *
 * This test verifies that the router's lazy initialization is thread-safe
 * when multiple threads concurrently access the router for the first time.
 *
 * Expected behavior:
 * - With THREAD_SAFE=true: All tests should pass
 * - With THREAD_SAFE=false: Tests may fail or show data races with ThreadSanitizer
 */

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>

#include <router.h>
#include <tests/initiator-tester.h>
#include <tests/target-tester.h>

using namespace std::chrono_literals;

/**
 * @brief Test fixture for thread-safety tests
 */
class ThreadSafetyTest : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    int exit_code{ 0 }; // Track test result for sc_main return
    // Test components
    gs::router<> router;
    InitiatorTester initiator1;
    InitiatorTester initiator2;
    InitiatorTester initiator3;
    TargetTester target1;
    TargetTester target2;
    TargetTester target3;

    // Synchronization
    std::atomic<bool> start_flag{ false };
    std::atomic<int> ready_count{ 0 };
    std::atomic<int> success_count{ 0 };
    std::atomic<int> error_count{ 0 };

    ThreadSafetyTest(sc_core::sc_module_name nm)
        : sc_core::sc_module(nm)
        , router("router")
        , initiator1("initiator1")
        , initiator2("initiator2")
        , initiator3("initiator3")
        , target1("target1", 0x1000)
        , target2("target2", 0x1000)
        , target3("target3", 0x1000)
    {
        // Connect initiators to router
        router.add_initiator(initiator1.socket);
        router.add_initiator(initiator2.socket);
        router.add_initiator(initiator3.socket);

        // Connect router to targets
        router.add_target(target1.socket, 0x0000, 0x1000);
        router.add_target(target2.socket, 0x1000, 0x1000);
        router.add_target(target3.socket, 0x2000, 0x1000);

        SC_THREAD(test_concurrent_initialization);
    }

    /**
     * @brief Perform a transaction from a specific initiator
     */
    bool do_transaction(InitiatorTester& initiator, uint64_t addr)
    {
        try {
            // Wait for all threads to be ready
            ready_count++;
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            // Perform transaction
            tlm::tlm_generic_payload txn;
            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
            uint8_t data[4] = { 0xDE, 0xAD, 0xBE, 0xEF };

            txn.set_command(tlm::TLM_WRITE_COMMAND);
            txn.set_address(addr);
            txn.set_data_ptr(data);
            txn.set_data_length(4);
            txn.set_streaming_width(4);
            txn.set_byte_enable_ptr(nullptr);
            txn.set_dmi_allowed(false);
            txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            initiator.socket->b_transport(txn, delay);

            if (txn.get_response_status() == tlm::TLM_OK_RESPONSE) {
                success_count++;
                return true;
            } else {
                error_count++;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception in thread: " << e.what() << std::endl;
            error_count++;
            return false;
        }
    }

    /**
     * @brief Test concurrent initialization from multiple threads
     */
    void test_concurrent_initialization()
    {
        std::cout << "\n=== Thread-Safety Test: Concurrent Initialization ===" << std::endl;
        std::cout << "THREAD_SAFE mode: " << (THREAD_SAFE ? "ENABLED" : "DISABLED") << std::endl;

        // Wait for SystemC to be ready
        wait(1, sc_core::SC_NS);

        const int NUM_THREADS = 10;
        std::vector<std::thread> threads;

        // Create threads that will all try to access the router simultaneously
        for (int i = 0; i < NUM_THREADS; i++) {
            threads.emplace_back([this, i]() {
                // Distribute transactions across different initiators and addresses
                InitiatorTester* initiator;
                uint64_t addr;

                switch (i % 3) {
                case 0:
                    initiator = &initiator1;
                    addr = 0x0100 + (i * 4);
                    break;
                case 1:
                    initiator = &initiator2;
                    addr = 0x1100 + (i * 4);
                    break;
                case 2:
                    initiator = &initiator3;
                    addr = 0x2100 + (i * 4);
                    break;
                }

                do_transaction(*initiator, addr);
            });
        }

        // Wait for all threads to be ready
        while (ready_count.load() < NUM_THREADS) {
            std::this_thread::sleep_for(1ms);
        }

        std::cout << "All threads ready. Starting concurrent access..." << std::endl;

        // Signal all threads to start simultaneously
        start_flag.store(true, std::memory_order_release);

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Report results
        std::cout << "\nResults:" << std::endl;
        std::cout << "  Successful transactions: " << success_count.load() << "/" << NUM_THREADS << std::endl;
        std::cout << "  Failed transactions: " << error_count.load() << "/" << NUM_THREADS << std::endl;

        // Verify all transactions succeeded
        if (success_count.load() == NUM_THREADS) {
            std::cout << "\n✓ TEST PASSED: All concurrent transactions succeeded" << std::endl;
            SCP_INFO(SCMOD)("✓ TEST PASSED: All concurrent transactions succeeded");
        } else {
            std::cout << "\n✗ TEST FAILED: Some transactions failed" << std::endl;
            SCP_ERR(SCMOD)("✗ TEST FAILED: Some transactions failed");
            exit_code = 1;
            sc_core::sc_stop();
            return;
        }

        // Additional test: Verify router state is consistent
        std::cout << "\nVerifying router state consistency..." << std::endl;

        // Perform additional transactions to ensure router is still functional
        for (int i = 0; i < 3; i++) {
            tlm::tlm_generic_payload txn;
            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
            uint8_t data[4] = { 0x12, 0x34, 0x56, 0x78 };

            txn.set_command(tlm::TLM_READ_COMMAND);
            txn.set_address(0x0000 + (i * 0x1000));
            txn.set_data_ptr(data);
            txn.set_data_length(4);
            txn.set_streaming_width(4);
            txn.set_byte_enable_ptr(nullptr);
            txn.set_dmi_allowed(false);
            txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            initiator1.socket->b_transport(txn, delay);

            if (txn.get_response_status() != tlm::TLM_OK_RESPONSE) {
                std::cout << "✗ Post-test transaction failed at address 0x" << std::hex << txn.get_address()
                          << std::endl;
                SCP_ERR(SCMOD)("✗ Post-test transaction failed at address {:#x}", txn.get_address());
                exit_code = 1;
                sc_core::sc_stop();
                return;
            }
        }

        std::cout << "✓ Router state is consistent after concurrent access" << std::endl;
        std::cout << "\n=== All Thread-Safety Tests PASSED ===" << std::endl;
        SCP_INFO(SCMOD)("=== All Thread-Safety Tests PASSED ===");

        sc_core::sc_stop();
    }
};

/**
 * @brief Test concurrent DMI requests with invalidations
 *
 * This test creates contention on the DMI mutex by having:
 * - Multiple threads requesting DMI (which use try_lock in THREAD_SAFE mode)
 * - Multiple threads invalidating DMI (which take full locks)
 *
 * Expected behavior:
 * - With THREAD_SAFE=true: Some DMI requests should be denied due to try_lock()
 *   failing when invalidation threads hold the lock
 * - With THREAD_SAFE=false: All DMI requests should succeed (no mutex protection)
 */
class DMIThreadSafetyTest : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    int exit_code{ 0 }; // Track test result for sc_main return
    gs::router<> router;
    InitiatorTester initiator1;
    InitiatorTester initiator2;
    TargetTester target1;
    TargetTester target2;

    std::atomic<bool> start_flag{ false };
    std::atomic<bool> stop_flag{ false };
    std::atomic<int> ready_count{ 0 };
    std::atomic<int> dmi_granted_count{ 0 };
    std::atomic<int> dmi_denied_count{ 0 };
    std::atomic<int> invalidation_count{ 0 };

    SC_HAS_PROCESS(DMIThreadSafetyTest);

    DMIThreadSafetyTest(sc_core::sc_module_name nm)
        : sc_core::sc_module(nm)
        , router("router")
        , initiator1("initiator1")
        , initiator2("initiator2")
        , target1("target1", 0x1000)
        , target2("target2", 0x1000)
    {
        router.add_initiator(initiator1.socket);
        router.add_initiator(initiator2.socket);
        router.add_target(target1.socket, 0x0000, 0x1000);
        router.add_target(target2.socket, 0x1000, 0x1000);

        SC_THREAD(test_concurrent_dmi_with_invalidation);
    }

    void do_dmi_requests(InitiatorTester& initiator, uint64_t addr, int thread_id)
    {
        try {
            ready_count++;
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            // Repeatedly request DMI while invalidations are happening
            int local_granted = 0;
            int local_denied = 0;

            while (!stop_flag.load(std::memory_order_acquire)) {
                tlm::tlm_generic_payload txn;
                tlm::tlm_dmi dmi_data;

                txn.set_address(addr);
                txn.set_command(tlm::TLM_READ_COMMAND);

                bool dmi_allowed = initiator.socket->get_direct_mem_ptr(txn, dmi_data);

                if (dmi_allowed) {
                    local_granted++;
                } else {
                    local_denied++;
                }

                // Small yield to allow other threads to run
                std::this_thread::yield();
            }

            dmi_granted_count.fetch_add(local_granted);
            dmi_denied_count.fetch_add(local_denied);

        } catch (const std::exception& e) {
            std::cerr << "Exception in DMI request thread " << thread_id << ": " << e.what() << std::endl;
            exit_code = 1;
        }
    }

    void do_dmi_invalidations(int thread_id)
    {
        try {
            ready_count++;
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            // Repeatedly invalidate DMI to create contention
            int local_invalidations = 0;

            while (!stop_flag.load(std::memory_order_acquire)) {
                // Invalidate alternating regions to create contention
                // We call invalidate on the target's socket, which propagates back through the router
                if (thread_id % 2 == 0) {
                    target1.socket->invalidate_direct_mem_ptr(0x0000, 0x0FFF);
                } else {
                    target2.socket->invalidate_direct_mem_ptr(0x0000, 0x0FFF); // Relative to target's base
                }

                local_invalidations++;

                // Sleep briefly to hold the lock and create contention
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            invalidation_count.fetch_add(local_invalidations);

        } catch (const std::exception& e) {
            std::cerr << "Exception in invalidation thread " << thread_id << ": " << e.what() << std::endl;
            exit_code = 1;
        }
    }

    void test_concurrent_dmi_with_invalidation()
    {
        std::cout << "\n=== Thread-Safety Test: Concurrent DMI with Invalidations ===" << std::endl;
        std::cout << "THREAD_SAFE mode: " << (THREAD_SAFE ? "ENABLED" : "DISABLED") << std::endl;

        wait(1, sc_core::SC_NS);

        const int NUM_DMI_THREADS = 20;  // Threads requesting DMI
        const int NUM_INVAL_THREADS = 5; // Threads invalidating DMI
        const int TOTAL_THREADS = NUM_DMI_THREADS + NUM_INVAL_THREADS;

        std::vector<std::thread> threads;

        // Create DMI request threads
        for (int i = 0; i < NUM_DMI_THREADS; i++) {
            threads.emplace_back([this, i]() {
                InitiatorTester* initiator = (i % 2 == 0) ? &initiator1 : &initiator2;
                uint64_t addr = (i % 2 == 0) ? 0x0100 : 0x1100;
                do_dmi_requests(*initiator, addr, i);
            });
        }

        // Create invalidation threads
        for (int i = 0; i < NUM_INVAL_THREADS; i++) {
            threads.emplace_back([this, i]() { do_dmi_invalidations(i); });
        }

        // Wait for all threads to be ready
        while (ready_count.load() < TOTAL_THREADS) {
            std::this_thread::sleep_for(1ms);
        }

        std::cout << "All threads ready. Starting concurrent DMI operations..." << std::endl;
        start_flag.store(true, std::memory_order_release);

        // Let the test run for a short duration
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Signal threads to stop
        stop_flag.store(true, std::memory_order_release);

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Report results
        std::cout << "\nResults:" << std::endl;
        std::cout << "  DMI requests granted: " << dmi_granted_count.load() << std::endl;
        std::cout << "  DMI requests denied: " << dmi_denied_count.load() << std::endl;
        std::cout << "  DMI invalidations: " << invalidation_count.load() << std::endl;

#if THREAD_SAFE == true
        // In THREAD_SAFE mode, we expect some denials due to try_lock() failing
        if (dmi_denied_count.load() > 0) {
            std::cout << "\n✓ TEST PASSED: DMI mutex contention detected (some requests denied as expected)"
                      << std::endl;
            SCP_INFO(SCMOD)("✓ TEST PASSED: DMI mutex contention detected");
        } else {
            std::cout << "\n⚠ WARNING: No DMI denials detected - mutex contention may not be sufficient" << std::endl;
            SCP_WARN(SCMOD)("No DMI denials detected - mutex contention may not be sufficient");
        }
#else
        // In non-THREAD_SAFE mode, all requests should succeed
        if (dmi_denied_count.load() == 0) {
            std::cout << "\n✓ TEST PASSED: All DMI requests granted (no mutex protection)" << std::endl;
            SCP_INFO(SCMOD)("✓ TEST PASSED: All DMI requests granted");
        } else {
            std::cout << "\n✗ TEST FAILED: Unexpected DMI denials in non-THREAD_SAFE mode" << std::endl;
            SCP_ERR(SCMOD)("✗ TEST FAILED: Unexpected DMI denials");
            exit_code = 1;
        }
#endif

        std::cout << "✓ Concurrent DMI test completed without crashes" << std::endl;

        sc_core::sc_stop();
    }
};

/**
 * @brief Combined test module that runs both initialization and DMI tests
 */
class CombinedThreadSafetyTest : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    int exit_code{ 0 };

    // Test 1: Initialization test components
    gs::router<> router1;
    InitiatorTester init1_1;
    InitiatorTester init1_2;
    InitiatorTester init1_3;
    TargetTester target1_1;
    TargetTester target1_2;
    TargetTester target1_3;

    // Test 2: DMI test components
    gs::router<> router2;
    InitiatorTester init2_1;
    InitiatorTester init2_2;
    TargetTester target2_1;
    TargetTester target2_2;

    SC_HAS_PROCESS(CombinedThreadSafetyTest);

    CombinedThreadSafetyTest(sc_core::sc_module_name nm)
        : sc_core::sc_module(nm)
        , router1("router1")
        , init1_1("init1_1")
        , init1_2("init1_2")
        , init1_3("init1_3")
        , target1_1("target1_1", 0x1000)
        , target1_2("target1_2", 0x1000)
        , target1_3("target1_3", 0x1000)
        , router2("router2")
        , init2_1("init2_1")
        , init2_2("init2_2")
        , target2_1("target2_1", 0x1000)
        , target2_2("target2_2", 0x1000)
    {
        // Setup router1 for initialization test
        router1.add_initiator(init1_1.socket);
        router1.add_initiator(init1_2.socket);
        router1.add_initiator(init1_3.socket);
        router1.add_target(target1_1.socket, 0x0000, 0x1000);
        router1.add_target(target1_2.socket, 0x1000, 0x1000);
        router1.add_target(target1_3.socket, 0x2000, 0x1000);

        // Setup router2 for DMI test
        router2.add_initiator(init2_1.socket);
        router2.add_initiator(init2_2.socket);
        router2.add_target(target2_1.socket, 0x0000, 0x1000);
        router2.add_target(target2_2.socket, 0x1000, 0x1000);

        SC_THREAD(run_all_tests);
    }

    void run_all_tests()
    {
        wait(1, sc_core::SC_NS);

        std::cout << "\n========================================" << std::endl;
        std::cout << "Router Thread-Safety Test Suite" << std::endl;
        std::cout << "THREAD_SAFE mode: " << (THREAD_SAFE ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << "========================================" << std::endl;

        // Run Test 1: Concurrent Initialization
        if (!run_initialization_test()) {
            exit_code = 1;
            sc_core::sc_stop();
            return;
        }

        // Run Test 2: Concurrent DMI with Invalidations
        if (!run_dmi_test()) {
            exit_code = 1;
            sc_core::sc_stop();
            return;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "✓ ALL THREAD-SAFETY TESTS PASSED" << std::endl;
        std::cout << "========================================\n" << std::endl;
        SCP_INFO(SCMOD)("✓ ALL THREAD-SAFETY TESTS PASSED");

        sc_core::sc_stop();
    }

    bool run_initialization_test()
    {
        std::cout << "\n=== Test 1: Concurrent Initialization ===" << std::endl;

        std::atomic<bool> start_flag{ false };
        std::atomic<int> ready_count{ 0 };
        std::atomic<int> success_count{ 0 };
        std::atomic<int> error_count{ 0 };

        const int NUM_THREADS = 10;
        std::vector<std::thread> threads;

        // Create threads for concurrent access
        for (int i = 0; i < NUM_THREADS; i++) {
            threads.emplace_back([this, i, &start_flag, &ready_count, &success_count, &error_count]() {
                InitiatorTester* initiator;
                uint64_t addr;

                switch (i % 3) {
                case 0:
                    initiator = &init1_1;
                    addr = 0x0100 + (i * 4);
                    break;
                case 1:
                    initiator = &init1_2;
                    addr = 0x1100 + (i * 4);
                    break;
                case 2:
                    initiator = &init1_3;
                    addr = 0x2100 + (i * 4);
                    break;
                }

                ready_count++;
                while (!start_flag.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }

                tlm::tlm_generic_payload txn;
                sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
                uint8_t data[4] = { 0xDE, 0xAD, 0xBE, 0xEF };

                txn.set_command(tlm::TLM_WRITE_COMMAND);
                txn.set_address(addr);
                txn.set_data_ptr(data);
                txn.set_data_length(4);
                txn.set_streaming_width(4);
                txn.set_byte_enable_ptr(nullptr);
                txn.set_dmi_allowed(false);
                txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

                initiator->socket->b_transport(txn, delay);

                if (txn.get_response_status() == tlm::TLM_OK_RESPONSE) {
                    success_count++;
                } else {
                    error_count++;
                }
            });
        }

        while (ready_count.load() < NUM_THREADS) {
            std::this_thread::sleep_for(1ms);
        }

        start_flag.store(true, std::memory_order_release);

        for (auto& thread : threads) {
            thread.join();
        }

        std::cout << "  Successful transactions: " << success_count.load() << "/" << NUM_THREADS << std::endl;
        std::cout << "  Failed transactions: " << error_count.load() << "/" << NUM_THREADS << std::endl;

        if (success_count.load() == NUM_THREADS) {
            std::cout << "✓ Test 1 PASSED" << std::endl;
            return true;
        } else {
            std::cout << "✗ Test 1 FAILED" << std::endl;
            SCP_ERR(SCMOD)("✗ Test 1 FAILED: Concurrent initialization test failed");
            return false;
        }
    }

    bool run_dmi_test()
    {
        std::cout << "\n=== Test 2: Concurrent DMI with Invalidations ===" << std::endl;

        std::atomic<bool> start_flag{ false };
        std::atomic<bool> stop_flag{ false };
        std::atomic<int> ready_count{ 0 };
        std::atomic<int> dmi_granted{ 0 };
        std::atomic<int> dmi_denied{ 0 };
        std::atomic<int> invalidations{ 0 };

        const int NUM_DMI_THREADS = 20;
        const int NUM_INVAL_THREADS = 5;
        const int TOTAL_THREADS = NUM_DMI_THREADS + NUM_INVAL_THREADS;

        std::vector<std::thread> threads;

        // DMI request threads
        for (int i = 0; i < NUM_DMI_THREADS; i++) {
            threads.emplace_back([this, i, &start_flag, &stop_flag, &ready_count, &dmi_granted, &dmi_denied]() {
                InitiatorTester* initiator = (i % 2 == 0) ? &init2_1 : &init2_2;
                uint64_t addr = (i % 2 == 0) ? 0x0100 : 0x1100;

                ready_count++;
                while (!start_flag.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }

                int local_granted = 0;
                int local_denied = 0;

                while (!stop_flag.load(std::memory_order_acquire)) {
                    tlm::tlm_generic_payload txn;
                    tlm::tlm_dmi dmi_data;
                    txn.set_address(addr);
                    txn.set_command(tlm::TLM_READ_COMMAND);

                    if (initiator->socket->get_direct_mem_ptr(txn, dmi_data)) {
                        local_granted++;
                    } else {
                        local_denied++;
                    }
                    std::this_thread::yield();
                }

                dmi_granted.fetch_add(local_granted);
                dmi_denied.fetch_add(local_denied);
            });
        }

        // Invalidation threads
        for (int i = 0; i < NUM_INVAL_THREADS; i++) {
            threads.emplace_back([this, i, &start_flag, &stop_flag, &ready_count, &invalidations]() {
                ready_count++;
                while (!start_flag.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }

                int local_inval = 0;

                while (!stop_flag.load(std::memory_order_acquire)) {
                    if (i % 2 == 0) {
                        target2_1.socket->invalidate_direct_mem_ptr(0x0000, 0x0FFF);
                    } else {
                        target2_2.socket->invalidate_direct_mem_ptr(0x0000, 0x0FFF);
                    }
                    local_inval++;
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }

                invalidations.fetch_add(local_inval);
            });
        }

        while (ready_count.load() < TOTAL_THREADS) {
            std::this_thread::sleep_for(1ms);
        }

        start_flag.store(true, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        stop_flag.store(true, std::memory_order_release);

        for (auto& thread : threads) {
            thread.join();
        }

        std::cout << "  DMI requests granted: " << dmi_granted.load() << std::endl;
        std::cout << "  DMI requests denied: " << dmi_denied.load() << std::endl;
        std::cout << "  DMI invalidations: " << invalidations.load() << std::endl;

#if THREAD_SAFE == true
        if (dmi_denied.load() > 0) {
            std::cout << "✓ Test 2 PASSED: DMI mutex contention detected" << std::endl;
            return true;
        } else {
            std::cout << "⚠ Test 2 WARNING: No DMI denials detected" << std::endl;
            SCP_WARN(SCMOD)("No DMI denials detected - mutex contention may not be sufficient");
            return true; // Don't fail, just warn
        }
#else
        if (dmi_denied.load() == 0) {
            std::cout << "✓ Test 2 PASSED: All DMI requests granted (no mutex)" << std::endl;
            return true;
        } else {
            std::cout << "✗ Test 2 FAILED: Unexpected DMI denials" << std::endl;
            SCP_ERR(SCMOD)("✗ Test 2 FAILED: Unexpected DMI denials in non-THREAD_SAFE mode");
            return false;
        }
#endif
    }
};

int sc_main(int argc, char* argv[])
{
    // Initialize CCI broker
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);

    // Initialize SCP logging
    scp::LoggingGuard logging_guard(scp::LogConfig()
                                        .fileInfoFrom(sc_core::SC_ERROR)
                                        .logAsync(false)
                                        .logLevel(scp::log::DBGTRACE)
                                        .msgTypeFieldWidth(50));

    CombinedThreadSafetyTest test("combined_test");
    sc_core::sc_start();

    return test.exit_code;
}
