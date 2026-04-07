/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <chrono>
#include <router.h>
#include <addrmap_cache_examples.h>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <scp/report.h>
#include <random>
#include <iostream>
#include <iomanip>
#include <vector>

using gs::router;
using tlm::tlm_dmi;
using namespace sc_core;
using namespace std::chrono;

// Configuration
static const unsigned int NUM_TARGETS = 10000;        // Large number of targets for realistic map lookup overhead
static const uint64_t TARGET_REGION_SIZE = 0x1000;    // 4KB per target (typical peripheral register size)
static const unsigned int NUM_TRANSACTIONS = 1000000; // 1M transactions for statistical significance
static const unsigned int CACHE_SIZE = 16;            // RegionCache size (after fix)
static const unsigned int THRASHING_SIZE = 32;        // 2x cache size for worst-case test

// --- Test Components ---

enum class CacheMode {
    NONE,
    LRU,
    REGION_FIFO,
    REGION_LRU,
};

enum class AccessPattern {
    REALISTIC_MIXED,
    BEST_CASE_CACHEABLE,
    WORST_CASE_UNCACHEABLE,
};

const char* to_string(CacheMode mode)
{
    switch (mode) {
    case CacheMode::NONE:
        return "NoCache";
    case CacheMode::LRU:
        return "LruCache";
    case CacheMode::REGION_FIFO:
        return "RegionFIFOCache";
    case CacheMode::REGION_LRU:
        return "RegionLRUCache";
    default:
        return "Unknown";
    }
}

const char* to_string(AccessPattern pattern)
{
    switch (pattern) {
    case AccessPattern::REALISTIC_MIXED:
        return "Realistic Mixed";
    case AccessPattern::BEST_CASE_CACHEABLE:
        return "Best-Case Cacheable";
    case AccessPattern::WORST_CASE_UNCACHEABLE:
        return "Worst-Case Uncacheable";
    default:
        return "Unknown";
    }
}

struct BenchmarkResult {
    double transactions_per_second = 0;
    double hit_rate = 0;
    uint64_t cache_hits = 0;
    uint64_t cache_misses = 0;
};

// A simple TLM target that does nothing but respond OK
class TestTarget : public sc_module
{
public:
    tlm_utils::simple_target_socket<TestTarget> target_socket;
    SC_CTOR (TestTarget) : target_socket("target_socket") {
        target_socket.register_b_transport(this, &TestTarget::b_transport);
    }
    void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay)
    {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
};

// --- The TestBench with Multi-Router Architecture ---
SC_MODULE (TestBench) {
public:
    // Four routers, one for each cache type
    router<32, gs::AddrMapNoCache> m_router_nocache;
    router<32, gs::LRUCache> m_router_lru;
    router<32, gs::RegionFIFOCache> m_router_region_fifo;
    router<32, gs::RegionLRUCache> m_router_region_lru;

    // Four initiator sockets, one for each router
    tlm_utils::simple_initiator_socket<TestBench> m_initiator_nocache;
    tlm_utils::simple_initiator_socket<TestBench> m_initiator_lru;
    tlm_utils::simple_initiator_socket<TestBench> m_initiator_region_fifo;
    tlm_utils::simple_initiator_socket<TestBench> m_initiator_region_lru;

    SC_CTOR (TestBench)
        : m_router_nocache("router_nocache")
        , m_router_lru("router_lru")
        , m_router_region_fifo("router_region_fifo")
        , m_router_region_lru("router_region_lru")
        , m_initiator_nocache("initiator_nocache")
        , m_initiator_lru("initiator_lru")
        , m_initiator_region_fifo("initiator_region_fifo")
        , m_initiator_region_lru("initiator_region_lru")
        , m_rng_state(12345ULL)  // Seed for LCG
        {
            // 1. Bind initiators to their respective routers
            m_initiator_nocache.bind(m_router_nocache.target_socket);
            m_initiator_lru.bind(m_router_lru.target_socket);
            m_initiator_region_fifo.bind(m_router_region_fifo.target_socket);
            m_initiator_region_lru.bind(m_router_region_lru.target_socket);

            // 2. Create separate test targets for each router (SystemC sockets can only be bound once)
            for (unsigned int i = 0; i < NUM_TARGETS; ++i) {
                uint64_t base_addr = TARGET_REGION_SIZE * i;

                // Create target for NoCache router
                std::string name_nocache = "target_nocache_" + std::to_string(i);
                m_targets_nocache.push_back(std::make_unique<TestTarget>(name_nocache.c_str()));
                m_router_nocache.add_target(m_targets_nocache.back()->target_socket, base_addr, TARGET_REGION_SIZE,
                                            false, 100);

                // Create target for LRU router
                std::string name_lru = "target_lru_" + std::to_string(i);
                m_targets_lru.push_back(std::make_unique<TestTarget>(name_lru.c_str()));
                m_router_lru.add_target(m_targets_lru.back()->target_socket, base_addr, TARGET_REGION_SIZE, false, 100);

                // Create target for RegionFIFO router
                std::string name_region_fifo = "target_region_fifo_" + std::to_string(i);
                m_targets_region_fifo.push_back(std::make_unique<TestTarget>(name_region_fifo.c_str()));
                m_router_region_fifo.add_target(m_targets_region_fifo.back()->target_socket, base_addr,
                                                TARGET_REGION_SIZE, false, 100);

                // Create target for RegionLRU router
                std::string name_region_lru = "target_region_lru_" + std::to_string(i);
                m_targets_region_lru.push_back(std::make_unique<TestTarget>(name_region_lru.c_str()));
                m_router_region_lru.add_target(m_targets_region_lru.back()->target_socket, base_addr,
                                               TARGET_REGION_SIZE, false, 100);
            }

            // 3. Register the main test thread
            SC_THREAD(run_all_benchmarks);
        }

private:
    std::vector<std::unique_ptr<TestTarget>> m_targets_nocache;
    std::vector<std::unique_ptr<TestTarget>> m_targets_lru;
    std::vector<std::unique_ptr<TestTarget>> m_targets_region_fifo;
    std::vector<std::unique_ptr<TestTarget>> m_targets_region_lru;
    std::vector<uint64_t> m_addresses;

    // Simple LCG for fast pseudo-random number generation
    uint64_t m_rng_state;

    // Fast pseudo-random number generator using Linear Congruential Generator
    uint64_t fast_rand()
    {
        // LCG parameters from Numerical Recipes
        m_rng_state = m_rng_state * 1664525ULL + 1013904223ULL;
        return m_rng_state;
    }

    // Generate random number in range [0, max)
    uint64_t fast_rand_range(uint64_t max) { return fast_rand() % max; }

    /**
     * @brief The main SC_THREAD that orchestrates the entire benchmark suite.
     */
    void run_all_benchmarks()
    {
        std::cout << std::endl;
        std::cout << "============================================================================================"
                  << std::endl;
        std::cout << "                            ROUTER CACHE PERFORMANCE BENCHMARK" << std::endl;
        std::cout << "============================================================================================"
                  << std::endl;
        std::cout << std::left << std::setw(25) << "Access Pattern" << std::setw(20) << "Cache Mode" << std::setw(20)
                  << "Transactions/sec" << std::setw(15) << "Hit Rate" << std::setw(12) << "Hits" << std::setw(12)
                  << "Misses" << std::endl;
        std::cout << "--------------------------------------------------------------------------------------------"
                  << std::endl;

        const std::vector<AccessPattern> access_patterns = {
            AccessPattern::REALISTIC_MIXED,
            AccessPattern::BEST_CASE_CACHEABLE,
            AccessPattern::WORST_CASE_UNCACHEABLE,
        };

        const std::vector<CacheMode> cache_modes = { CacheMode::NONE, CacheMode::LRU, CacheMode::REGION_FIFO,
                                                     CacheMode::REGION_LRU };

        // Run all combinations of access patterns and cache modes
        for (auto pattern : access_patterns) {
            for (auto mode : cache_modes) {
                auto result = run_single_benchmark(pattern, mode);
                print_results(pattern, mode, result);
            }
            // Add separator between access patterns
            if (pattern != access_patterns.back()) {
                std::cout
                    << "--------------------------------------------------------------------------------------------"
                    << std::endl;
            }
        }

        std::cout << "============================================================================================"
                  << std::endl;
        std::cout << "Benchmark complete." << std::endl;
        sc_stop();
    }

    /**
     * @brief Runs a single benchmark configuration and returns the results.
     */
    BenchmarkResult run_single_benchmark(AccessPattern pattern, CacheMode mode)
    {
        generate_addresses(pattern);

        // Reset cache statistics before measurement
        reset_cache_stats(mode);

        // Warm-up phase (first transaction also triggers lazy_initialize)
        for (unsigned int i = 0; i < std::min((unsigned int)m_addresses.size(), 1000u); ++i) {
            perform_transaction(m_addresses[i], mode);
        }

        // Reset statistics again after warm-up
        reset_cache_stats(mode);

        // Timed measurement phase
        auto start_time = high_resolution_clock::now();
        for (uint64_t addr : m_addresses) {
            perform_transaction(addr, mode);
        }
        auto end_time = high_resolution_clock::now();

        duration<double> time_span = duration_cast<duration<double>>(end_time - start_time);

        BenchmarkResult result;
        result.transactions_per_second = m_addresses.size() / time_span.count();

        // Get cache statistics
        get_cache_stats(mode, result.cache_hits, result.cache_misses);

        // Calculate hit rate
        uint64_t total_accesses = result.cache_hits + result.cache_misses;
        if (total_accesses > 0) {
            result.hit_rate = static_cast<double>(result.cache_hits) / total_accesses;
        }

        return result;
    }

    /**
     * @brief Generates a list of addresses based on the specified access pattern.
     */
    void generate_addresses(AccessPattern pattern)
    {
        m_addresses.clear();
        m_addresses.reserve(NUM_TRANSACTIONS);

        switch (pattern) {
        case AccessPattern::REALISTIC_MIXED: {
            // Pattern 1: Realistic Mixed Workload
            // 70% hot peripheral registers, 20% memory with locality, 10% cold peripherals

            // Define hot peripheral addresses (3 targets × 12 registers each = 36 addresses)
            std::vector<uint64_t> hot_registers;
            for (unsigned int target = 0; target < 3; target++) {
                for (unsigned int reg = 0; reg < 12; reg++) {
                    hot_registers.push_back(TARGET_REGION_SIZE * target + reg * 4);
                }
            }

            // Memory working set (target 10, 1000 addresses with 4-byte spacing)
            std::vector<uint64_t> memory_working_set;
            uint64_t mem_target = 10;
            for (unsigned int i = 0; i < 1000; i++) {
                memory_working_set.push_back(TARGET_REGION_SIZE * mem_target + i * 4);
            }

            // Pre-generate random values for cold accesses (10% of transactions)
            const unsigned int NUM_COLD = NUM_TRANSACTIONS / 10;
            std::vector<uint64_t> cold_addresses;
            cold_addresses.reserve(NUM_COLD);
            for (unsigned int i = 0; i < NUM_COLD; ++i) {
                unsigned int cold_target = fast_rand_range(NUM_TARGETS);
                uint64_t cold_offset = fast_rand_range(TARGET_REGION_SIZE);
                cold_addresses.push_back(TARGET_REGION_SIZE * cold_target + cold_offset);
            }

            // Generate accesses with 70/20/10 distribution
            unsigned int cold_idx = 0;

            for (unsigned int i = 0; i < NUM_TRANSACTIONS; ++i) {
                // Use simple modulo for distribution (fast approximation)
                uint64_t r = fast_rand() % 100;
                if (r < 70) {
                    // 70%: Hot peripheral register (with repetition)
                    // Use only first 12 addresses to fit in 16-entry LRU cache
                    m_addresses.push_back(hot_registers[i % 12]);
                } else if (r < 90) {
                    // 20%: Memory working set (with repetition)
                    m_addresses.push_back(memory_working_set[i % memory_working_set.size()]);
                } else {
                    // 10%: Cold peripheral (pre-generated random addresses)
                    m_addresses.push_back(cold_addresses[cold_idx++ % cold_addresses.size()]);
                }
            }
            break;
        }

        case AccessPattern::BEST_CASE_CACHEABLE: {
            // Pattern 2: Best-Case Cacheable
            // 8 register addresses in single target, repeatedly cycled
            std::vector<uint64_t> registers = { 0x0, 0x4, 0x8, 0xC, 0x10, 0x14, 0x18, 0x1C };

            for (unsigned int i = 0; i < NUM_TRANSACTIONS; ++i) {
                m_addresses.push_back(registers[i % 8]);
            }
            break;
        }

        case AccessPattern::WORST_CASE_UNCACHEABLE: {
            // Pattern 3: Worst-Case Uncacheable
            //
            // Goal: Defeat BOTH caches by maximizing unique addresses and targets
            // - LRU: 65,536 entries (address-based)
            // - RegionCache: 8 entries (region-based)
            //
            // Strategy:
            // - Cycle through ALL 10,000 targets (way more than 8 RegionCache capacity)
            // - Generate 16 unique addresses per target (fits within 4KB region)
            // - Total unique addresses: 10,000 × 16 = 160,000 (2.4× LRU capacity of 65,536)

            const unsigned int ADDRS_PER_TARGET = 16; // Addresses per target (fits in 4KB)
            const unsigned int ADDR_SPACING = 256;    // 256 bytes spacing (16 * 256 = 4KB)

            for (unsigned int i = 0; i < NUM_TRANSACTIONS; ++i) {
                // Cycle through ALL targets to maximize RegionCache thrashing
                uint64_t target_idx = i % NUM_TARGETS;
                // Generate unique addresses per target to defeat LRU
                uint64_t addr_within_target = (i / NUM_TARGETS) % ADDRS_PER_TARGET;
                uint64_t offset = addr_within_target * ADDR_SPACING;
                m_addresses.push_back(TARGET_REGION_SIZE * target_idx + offset);
            }
            break;
        }
        }
    }

    /**
     * @brief Performs a single blocking transport call using the specified cache mode.
     */
    void perform_transaction(uint64_t address, CacheMode mode)
    {
        tlm::tlm_generic_payload trans;
        sc_time delay = SC_ZERO_TIME;
        unsigned char data;
        trans.set_command(tlm::TLM_WRITE_COMMAND);
        trans.set_address(address);
        trans.set_data_ptr(&data);
        trans.set_data_length(1);

        // Select the appropriate socket based on cache mode
        switch (mode) {
        case CacheMode::NONE:
            m_initiator_nocache->b_transport(trans, delay);
            break;
        case CacheMode::LRU:
            m_initiator_lru->b_transport(trans, delay);
            break;
        case CacheMode::REGION_FIFO:
            m_initiator_region_fifo->b_transport(trans, delay);
            break;
        case CacheMode::REGION_LRU:
            m_initiator_region_lru->b_transport(trans, delay);
            break;
        }

        if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
            SC_REPORT_ERROR("TestBench", "Transaction failed!");
        }
    }

    /**
     * @brief Gets cache statistics from the appropriate router.
     */
    void get_cache_stats(CacheMode mode, uint64_t& hits, uint64_t& misses)
    {
        switch (mode) {
        case CacheMode::NONE:
            m_router_nocache.get_cache_stats(hits, misses);
            break;
        case CacheMode::LRU:
            m_router_lru.get_cache_stats(hits, misses);
            break;
        case CacheMode::REGION_FIFO:
            m_router_region_fifo.get_cache_stats(hits, misses);
            break;
        case CacheMode::REGION_LRU:
            m_router_region_lru.get_cache_stats(hits, misses);
            break;
        }
    }

    /**
     * @brief Resets cache statistics for the appropriate router.
     */
    void reset_cache_stats(CacheMode mode)
    {
        switch (mode) {
        case CacheMode::NONE:
            m_router_nocache.reset_cache_stats();
            break;
        case CacheMode::LRU:
            m_router_lru.reset_cache_stats();
            break;
        case CacheMode::REGION_FIFO:
            m_router_region_fifo.reset_cache_stats();
            break;
        case CacheMode::REGION_LRU:
            m_router_region_lru.reset_cache_stats();
            break;
        }
    }

    /**
     * @brief Prints a formatted line of benchmark results to the console.
     */
    void print_results(AccessPattern pattern, CacheMode mode, const BenchmarkResult& result)
    {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << std::left << std::setw(25) << to_string(pattern) << std::setw(20) << to_string(mode)
                  << std::setw(20) << result.transactions_per_second << std::setw(15) << (result.hit_rate * 100)
                  << std::setw(12) << result.cache_hits << std::setw(12) << result.cache_misses << std::endl;
    }
};

/**
 * @brief sc_main: The entry point for the SystemC simulation.
 */
int sc_main(int argc, char* argv[])
{
    // Set up CCI and logging
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);
    scp::LoggingGuard logging_guard(scp::LogConfig().logLevel(scp::log::WARNING));

    // Instantiate the testbench with multi-router architecture
    TestBench tb("TestBench");

    // Run the simulation
    sc_start();

    return 0;
}
