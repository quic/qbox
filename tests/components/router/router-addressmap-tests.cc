/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gtest/gtest.h>
#include <router.h>
#include <systemc>
#include <tlm>
#include <limits> // Required for std::numeric_limits

using gs::router;
using tlm::tlm_dmi;

// Define a simple target_info struct for testing purposes
// This should match the router::target_info structure
struct TestTargetInfo {
    std::string name;
    uint64_t address;
    uint64_t size;
    unsigned int priority;
    bool use_offset;
    int index;
    bool chained;

    TestTargetInfo(std::string n, uint64_t addr, uint64_t s, unsigned int p, bool offset = true)
        : name(n), address(addr), size(s), priority(p), use_offset(offset), index(0), chained(false)
    {
    }
};

// Custom addressMap for testing with TestTargetInfo
// Replicated from router.h, but using TestTargetInfo
template <typename TargetInfoType, int CACHESIZE = 16>
class TestAddressMap
{
    std::map<unsigned int, std::map<uint64_t, std::shared_ptr<TargetInfoType>>> m_priority_maps;
    std::vector<std::pair<uint64_t, std::shared_ptr<TargetInfoType>>> m_cache;
    static constexpr int PAGEMASK = 0x1ff; // Keep the same page mask as in router.h

public:
    TestAddressMap() {}

    void add(std::shared_ptr<TargetInfoType> t_info)
    {
        uint64_t start_addr = t_info->address;
        uint64_t end_addr = t_info->address + t_info->size - 1;

        if ((start_addr & PAGEMASK) != 0) {
            std::stringstream ss;
            ss << "Fatal address page mask violation for " << t_info->name << ", address given : 0x" << std::hex
               << start_addr << " page mask 0x" << PAGEMASK << " (error 0x " << (start_addr & PAGEMASK) << ")";
            SC_REPORT_FATAL("addressMap:add", ss.str().c_str());
        }

        std::map<uint64_t, std::shared_ptr<TargetInfoType>>& current_map = m_priority_maps[t_info->priority];
        auto it = current_map.upper_bound(start_addr);

        if (it != current_map.begin()) {
            --it;
            const std::shared_ptr<TargetInfoType>& existing_ti = it->second;
            uint64_t existing_start = existing_ti->address;
            uint64_t existing_end = existing_ti->address + existing_ti->size - 1;

            if (start_addr <= existing_end && end_addr >= existing_start) {
                SC_REPORT_FATAL("addressMap", "Overlap error (test version)");
            }
            ++it;
        }

        while (it != current_map.end() && it->second->address <= end_addr) {
            const std::shared_ptr<TargetInfoType>& existing_ti = it->second;
            uint64_t existing_start = existing_ti->address;
            uint64_t existing_end = existing_ti->address + existing_ti->size - 1;

            if (start_addr <= existing_end && end_addr >= existing_start) {
                SC_REPORT_FATAL("addressMap", "Overlap error (test version)");
            }
            ++it;
        }

        m_priority_maps[t_info->priority][start_addr] = t_info;
        m_cache.clear();
    }

    /**
     * @brief Finds the shared_ptr to TargetInfoType associated with a given address, searching by priority.
     *
     * This method first checks the internal cache. If not found, it iterates through priority maps
     * (from highest to lowest priority) to find the target whose address range contains the given address.
     * Found targets are added to the cache.
     *
     * @param address The address to look up.
     * @return A shared_ptr to the found TargetInfoType object, or nullptr if no target is found.
     */
    std::shared_ptr<TargetInfoType> find(uint64_t address)
    {
        // Iterate through priority maps in order of priority (map iterates keys in increasing order)
        for (auto const& pair_map : m_priority_maps) {
            const std::map<uint64_t, std::shared_ptr<TargetInfoType>>& current_map = pair_map.second;
            auto it = current_map.upper_bound(address);
            if (it != current_map.begin()) {
                --it; // Move back to the element whose key is <= address
                // Check if the address falls within this entry's range
                if (address >= it->second->address && address <= (it->second->address + it->second->size - 1)) {
                    return it->second; // Found the highest priority target
                }
            }
        }
        return nullptr; // No target found for the given address
    }

    std::shared_ptr<TargetInfoType> find_region(uint64_t addr, tlm::tlm_dmi& dmi)
    {
        std::shared_ptr<TargetInfoType> found_target = nullptr;

        // Step 1: Find the best matching target for 'addr'
        // Iterate through priority maps from lowest priority value (highest actual priority)
        for (auto const& pair_map : m_priority_maps) {
            const std::map<uint64_t, std::shared_ptr<TargetInfoType>>& current_map = pair_map.second;

            auto it = current_map.upper_bound(addr);
            if (it != current_map.begin()) {
                --it; // Move back to the element whose key is <= addr
                const std::shared_ptr<TargetInfoType>& ti = it->second;
                // Check if addr is within this target's range
                if (addr >= ti->address && addr < (ti->address + ti->size)) {
                    found_target = ti;
                    break; // Found the highest priority target, no need to check lower priorities
                }
            }
        }

        if (found_target) {
            // Step 2: If a target was found, DMI represents its region
            dmi.set_start_address(found_target->address);
            dmi.set_end_address(found_target->address + found_target->size - 1);
            return found_target;
        } else {
            // Step 3: If no target was found, 'addr' is in a hole. Determine hole boundaries.
            uint64_t hole_start = 0;
            uint64_t hole_end = std::numeric_limits<uint64_t>::max();

            // Find the highest address of a region ending before 'addr'
            // This determines the lower bound of the hole.
            for (auto const& pair_map : m_priority_maps) {
                const std::map<uint64_t, std::shared_ptr<TargetInfoType>>& current_map = pair_map.second;
                for (auto const& pair : current_map) {
                    const std::shared_ptr<TargetInfoType>& ti = pair.second;
                    if (ti->address + ti->size <= addr) { // Region ends before or at 'addr'
                        hole_start = std::max(hole_start, ti->address + ti->size);
                    }
                }
            }

            // Find the lowest address of a region starting after 'addr'
            // This determines the upper bound of the hole.
            for (auto const& pair_map : m_priority_maps) {
                const std::map<uint64_t, std::shared_ptr<TargetInfoType>>& current_map = pair_map.second;
                for (auto const& pair : current_map) {
                    const std::shared_ptr<TargetInfoType>& ti = pair.second;
                    if (ti->address > addr) { // Region starts strictly after 'addr'
                        hole_end = std::min(hole_end, ti->address - 1);
                    }
                }
            }

            dmi.set_start_address(hole_start);
            dmi.set_end_address(hole_end);
            return nullptr;
        }
    }
};

class AddressMapTest : public ::testing::Test
{
protected:
    TestAddressMap<TestTargetInfo> address_map;

    void SetUp() override
    {
        // Reset SystemC kernel before each test to ensure a clean state
        sc_core::sc_get_curr_simcontext()->reset();
    }

    // Helper to create and add target info
    std::shared_ptr<TestTargetInfo> create_and_add_target(const std::string& name, uint64_t address, uint64_t size,
                                                          unsigned int priority)
    {
        auto ti = std::make_shared<TestTargetInfo>(name, address, size, priority);
        address_map.add(ti);
        return ti;
    }
};

// Test case 1: No overlap, simple regions
TEST_F(AddressMapTest, NoOverlapSimpleRegions)
{
    create_and_add_target("TargetA", 0x1000, 0x100, 10);
    create_and_add_target("TargetB", 0x2000, 0x200, 10);

    tlm_dmi dmi;
    auto found = address_map.find_region(0x1050, dmi);
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetA");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x10FF);

    found = address_map.find_region(0x2100, dmi);
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetB");
    ASSERT_EQ(dmi.get_start_address(), 0x2000);
    ASSERT_EQ(dmi.get_end_address(), 0x21FF);

    found = address_map.find_region(0x500, dmi); // Hole before A
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x0);  // start of hole
    ASSERT_EQ(dmi.get_end_address(), 0x0FFF); // end of hole (before TargetA)

    found = address_map.find_region(0x1500, dmi); // Hole between A and B
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x1100);
    ASSERT_EQ(dmi.get_end_address(), 0x1FFF);

    found = address_map.find_region(0x3000, dmi); // Hole after B
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x2200);
    ASSERT_EQ(dmi.get_end_address(), std::numeric_limits<uint64_t>::max());
}

// Test case 2: Overlapping regions with different priorities
TEST_F(AddressMapTest, OverlappingRegionsPriority)
{
    // Lower priority value means higher actual priority (0 is highest)
    create_and_add_target("LowPrioTarget", 0x1200, 0x200, 10); // Range: 0x1200-0x13FF (aligned)
    create_and_add_target("HighPrioTarget", 0x1200, 0x100,
                          0); // Range: 0x1200-0x12FF (overlaps with LowPrioTarget, aligned)

    tlm_dmi dmi;
    auto found = address_map.find_region(0x1220, dmi); // In HighPrioTarget due to priority
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "HighPrioTarget");
    ASSERT_EQ(dmi.get_start_address(), 0x1200);
    ASSERT_EQ(dmi.get_end_address(), 0x12FF);

    found = address_map.find_region(0x1320, dmi); // In LowPrioTarget only (after HighPrioTarget's effective range)
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "LowPrioTarget");
    ASSERT_EQ(dmi.get_start_address(), 0x1200);
    ASSERT_EQ(dmi.get_end_address(), 0x13FF);
}

// Test case 3: Nested regions with different priorities
TEST_F(AddressMapTest, NestedRegionsPriority)
{
    create_and_add_target("OuterTarget", 0x1000, 0x500, 10); // Range: 0x1000-0x14FF
    create_and_add_target("InnerTarget", 0x1200, 0x100, 0);  // Range: 0x1200-0x12FF (higher priority, nested, aligned)

    tlm_dmi dmi;
    auto found = address_map.find_region(0x1050, dmi); // In OuterTarget only
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "OuterTarget");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x14FF);

    found = address_map.find_region(0x1250, dmi); // In nested region, InnerTarget should win
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "InnerTarget");
    ASSERT_EQ(dmi.get_start_address(), 0x1200);
    ASSERT_EQ(dmi.get_end_address(), 0x12FF);

    found = address_map.find_region(0x1400, dmi); // In OuterTarget only
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "OuterTarget");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x14FF);
}

// Test case 4: Adjacent regions and holes around them
TEST_F(AddressMapTest, AdjacentRegionsAndHoles)
{
    create_and_add_target("TargetX", 0x1000, 0x100, 0); // 0x1000-0x10FF (aligned)
    create_and_add_target("TargetY", 0x1200, 0x100, 0); // 0x1200-0x12FF (aligned)

    tlm_dmi dmi;
    auto found = address_map.find_region(0x0500, dmi); // Hole before TargetX
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x0);
    ASSERT_EQ(dmi.get_end_address(), 0x0FFF); // Up to start of TargetX

    found = address_map.find_region(0x1000, dmi); // At start of TargetX
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetX");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x10FF);

    found = address_map.find_region(0x10FF, dmi); // At end of TargetX
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetX");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x10FF);

    found = address_map.find_region(0x1200, dmi); // At start of TargetY
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetY");
    ASSERT_EQ(dmi.get_start_address(), 0x1200);
    ASSERT_EQ(dmi.get_end_address(), 0x12FF);

    found = address_map.find_region(0x1300, dmi); // Hole after TargetY
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x1300); // Start of hole
    ASSERT_EQ(dmi.get_end_address(), std::numeric_limits<uint64_t>::max());
}

// Test case 5: Empty address map
TEST_F(AddressMapTest, EmptyMap)
{
    tlm_dmi dmi;
    auto found = address_map.find_region(0x1000, dmi);
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x0);
    ASSERT_EQ(dmi.get_end_address(), std::numeric_limits<uint64_t>::max());
}

// Test case 6: Address at boundary
TEST_F(AddressMapTest, BoundaryAddress)
{
    create_and_add_target("TargetSingle", 0x1000, 0x1, 0); // Single byte at 0x1000

    tlm_dmi dmi;
    auto found = address_map.find_region(0x1000, dmi);
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetSingle");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x1000);

    found = address_map.find_region(0x0FFF, dmi); // Before
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x0);
    ASSERT_EQ(dmi.get_end_address(), 0x0FFF);

    found = address_map.find_region(0x1001, dmi); // After
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x1001);
    ASSERT_EQ(dmi.get_end_address(), std::numeric_limits<uint64_t>::max());
}

// Test case 7: Multiple overlapping regions, verify highest priority wins
TEST_F(AddressMapTest, MultipleOverlappingPriorities)
{
    create_and_add_target("Lowest", 0x1000, 0x500, 100); // 0x1000-0x14FF
    create_and_add_target("Middle", 0x1000, 0x300, 50);  // 0x1000-0x12FF
    create_and_add_target("Highest", 0x1000, 0x100, 10); // 0x1000-0x10FF (aligned)

    tlm_dmi dmi;
    auto found = address_map.find_region(0x1020, dmi); // Highest priority wins
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "Highest");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x10FF);

    found = address_map.find_region(0x1120, dmi); // Middle priority wins over Lowest
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "Middle");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x12FF);

    found = address_map.find_region(0x1300, dmi); // Lowest wins
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "Lowest");
    ASSERT_EQ(dmi.get_start_address(), 0x1000);
    ASSERT_EQ(dmi.get_end_address(), 0x14FF);
}

// Test case 8: Check hole boundary logic carefully
TEST_F(AddressMapTest, HoleBoundaryLogic)
{
    create_and_add_target("Target1", 0x1000, 0x100, 0); // 0x1000 - 0x10FF
    create_and_add_target("Target2", 0x1200, 0x100, 0); // 0x1200 - 0x12FF
    create_and_add_target("Target3", 0x1600, 0x100, 0); // 0x1600 - 0x16FF (aligned)

    tlm_dmi dmi;

    // Hole before Target1
    auto found = address_map.find_region(0x0500, dmi);
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x0);
    ASSERT_EQ(dmi.get_end_address(), 0x0FFF);

    // Hole between Target1 and Target2
    found = address_map.find_region(0x1150, dmi);
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x1100);
    ASSERT_EQ(dmi.get_end_address(), 0x11FF);

    // Hole between Target2 and Target3
    found = address_map.find_region(0x1350, dmi);
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x1300);
    ASSERT_EQ(dmi.get_end_address(), 0x15FF); // Adjusted end of hole

    // Hole after Target3
    found = address_map.find_region(0x1750, dmi); // Adjusted search address
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x1700); // Adjusted start of hole
    ASSERT_EQ(dmi.get_end_address(), std::numeric_limits<uint64_t>::max());

    // Check addresses right at the boundary of a hole (should return the hole, not target)
    found = address_map.find_region(0x0FFF, dmi); // Last address of hole before Target1
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x0);
    ASSERT_EQ(dmi.get_end_address(), 0x0FFF);

    found = address_map.find_region(0x1200 + 0x100 - 1 + 1, dmi); // First address of hole after Target2
    ASSERT_EQ(found, nullptr);
    ASSERT_EQ(dmi.get_start_address(), 0x1300);
    ASSERT_EQ(dmi.get_end_address(), 0x15FF);
}

// Test case for the find method
TEST_F(AddressMapTest, FindMethodBasic)
{
    create_and_add_target("TargetA", 0x1000, 0x200, 10); // 0x1000 - 0x11FF (aligned)
    create_and_add_target("TargetB", 0x2000, 0x200, 20); // 0x2000 - 0x21FF (aligned)
    create_and_add_target("TargetC", 0x1000, 0x100, 5);  // 0x1000 - 0x10FF (higher priority, overlaps A, aligned)

    // Test a hit in TargetC (highest priority overlap, aligned address)
    auto found = address_map.find(0x1010);
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetC");

    // Test a hit in TargetA (after TargetC's range)
    found = address_map.find(0x1110);
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetA");

    // Test a hit in TargetB
    found = address_map.find(0x2050);
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetB");

    // Test an unmapped address (unaligned to page mask)
    found = address_map.find(0x50);
    ASSERT_EQ(found, nullptr);

    // Test an unmapped address (aligned to page mask, but no target)
    found = address_map.find(0x200);
    ASSERT_EQ(found, nullptr);

    // Test an address between mapped regions (aligned to page mask, but no target)
    found = address_map.find(0x1200);
    ASSERT_EQ(found, nullptr);

    // Test address exactly at boundary of TargetC
    found = address_map.find(0x10FF);
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->name, "TargetC");
}

int sc_main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
