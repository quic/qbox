/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "aliases-mapping-bench.h"

// Simple read into the memory
TEST_BENCH(AliasesMappingTest, SimpleWriteRead)
{
    uint8_t data;
    /* Target 1 (main) */
    ASSERT_EQ(m_initiator.do_write(0x0000, 0x04), tlm::TLM_OK_RESPONSE); // 0x0000 - 0x0FFF
    ASSERT_EQ(m_initiator.do_read(0x0000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 1 : alias1 (mapped to m_memory) */
    ASSERT_EQ(m_initiator.do_write(0x1000, 0x04), tlm::TLM_OK_RESPONSE); // This now maps to m_memory
    ASSERT_EQ(m_initiator.do_read(0x1000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 1 : alias2 (mapped to m_memory) */
    ASSERT_EQ(m_initiator.do_write(0x4000, 0x04), tlm::TLM_OK_RESPONSE); // This now maps to m_memory
    ASSERT_EQ(m_initiator.do_read(0x4000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 2 (m_ram) */
    ASSERT_EQ(m_initiator.do_write(0x5000, 0x04), tlm::TLM_OK_RESPONSE); // 0x5000 - 0x5FFF
    ASSERT_EQ(m_initiator.do_read(0x5000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 (m_rom) */
    ASSERT_EQ(m_initiator.do_write(0x2000, 0x04), tlm::TLM_OK_RESPONSE); // 0x2000 - 0x2FFF
    ASSERT_EQ(m_initiator.do_read(0x2000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 : alias1 (mapped to m_rom) */
    ASSERT_EQ(m_initiator.do_write(0x3000, 0x04), tlm::TLM_OK_RESPONSE); // This now maps to m_rom
    ASSERT_EQ(m_initiator.do_read(0x3000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);
}

// Transaction outside of the target address space
TEST_BENCH(AliasesMappingTest, SimpleOverlapWrite)
{
    /* Target 1 boundary check (0x0000 - 0x0FFF) */
    // 0x0FFF + 1 (0x1000) is the start of target.aliases.0, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0FFF + 1, 0x04), tlm::TLM_OK_RESPONSE);

    /* Target 1 alias1 boundary check (0x1000 - 0x1FFF) */
    // 0x1FFF + 1 (0x2000) is the start of m_rom, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x1FFF + 1, 0x04), tlm::TLM_OK_RESPONSE);

    /* Target 3 boundary check (0x2000 - 0x2FFF) */
    // 0x2FFF + 1 (0x3000) is the start of rom.aliases.alias1, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x2FFF + 1, 0x04), tlm::TLM_OK_RESPONSE);

    /* Target 3 alias1 boundary check (0x3000 - 0x3FFF) */
    // 0x3FFF + 1 (0x4000) is the start of target.aliases.1, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x3FFF + 1, 0x04), tlm::TLM_OK_RESPONSE);

    /* Target 1 alias2 boundary check (0x4000 - 0x4FFF) */
    // 0x4FFF + 1 (0x5000) is the start of m_ram, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x4FFF + 1, 0x04), tlm::TLM_OK_RESPONSE);

    /* Test for an address truly out of bounds */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x5FFF + 1, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);
}

// Transaction that crosses the boundary
TEST_BENCH(AliasesMappingTest, SimpleCrossesBoundary)
{
    /* Target 1 boundary check (0x0000 - 0x0FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x0FFF, 0xFFFF),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into alias at 0x1000
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x0FFF - 1, 0xFFFF), tlm::TLM_OK_RESPONSE);

    /* Target 1 alias1 boundary check (0x1000 - 0x1FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x1FFF, 0xFFFF),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into rom at 0x2000
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x1FFF - 1, 0xFFFF), tlm::TLM_OK_RESPONSE);

    /* Target 3 boundary check (0x2000 - 0x2FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x2FFF, 0xFFFF),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into rom alias at 0x3000
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x2FFF - 1, 0xFFFF), tlm::TLM_OK_RESPONSE);

    /* Target 3 alias1 boundary check (0x3000 - 0x3FFF) */
    ASSERT_EQ(
        m_initiator.do_write<uint16_t>(0x3FFF, 0xFFFF),
        tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into target alias2 at 0x4000
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x3FFF - 1, 0xFFFF), tlm::TLM_OK_RESPONSE);

    /* Target 1 alias2 boundary check (0x4000 - 0x4FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x4FFF, 0xFFFF),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into ram at 0x5000
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x4FFF - 1, 0xFFFF), tlm::TLM_OK_RESPONSE);

    /* Target 2 boundary check (0x5000 - 0x5FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x5FFF, 0xFFFF),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should be out of bounds
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x5FFF - 1, 0xFFFF), tlm::TLM_OK_RESPONSE);
}

TEST_BENCH(AliasesMappingTest, SimpleWriteReadDebug)
{
    uint8_t data8 = 0;
    uint16_t data16 = 0;
    uint32_t data32 = 0;
    uint64_t data64 = 0;

    // set some data so that we are sure we only get the right amount below
    ASSERT_EQ(m_initiator.do_write<uint64_t>(0, -1, true), tlm ::TLM_OK_RESPONSE);

    /* Target 1 main */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0000, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0000, data8, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data8));
    ASSERT_EQ(data8, 0x04);

    /* Target 1 : alias1 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x1000, 0x04, true),
              tlm ::TLM_OK_RESPONSE); // This is actually target alias1 (0x1000 maps to m_memory).
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0x1000, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);

    /* Target 1 : alias2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x4000, 0x04, true),
              tlm ::TLM_OK_RESPONSE); // This is actually target alias2 (0x4000 maps to m_memory).
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0x4000, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);

    /* Target 2 (m_ram) */
    ASSERT_EQ(m_initiator.do_write<uint64_t>(0x5000, 0x04, true),
              tlm ::TLM_OK_RESPONSE); // This is main m_ram, so no alias involved.
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint64_t));
    ASSERT_EQ(m_initiator.do_read(0x5000, data64, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data64));
    ASSERT_EQ(data64, 0x04);

    /* Target 3 main (m_rom) */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x2000, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x2000, data8, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data8));
    ASSERT_EQ(data8, 0x04);

    /* Target 3 : alias1 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x3000, 0x04, true),
              tlm ::TLM_OK_RESPONSE); // This is actually target alias1 (0x3000 maps to m_rom).
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0x3000, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);
}

// Debug Transport Interface transaction outside of the target address space
TEST_BENCH(AliasesMappingTest, SimpleOverlapWriteDebug)
{
    /* Target 1 boundary check (0x0000 - 0x0FFF) */
    // 0x0FFF + 1 (0x1000) is the start of target.aliases.0, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0FFF + 1, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));

    /* Target 1 alias1 boundary check (0x1000 - 0x1FFF) */
    // 0x1FFF + 1 (0x2000) is the start of m_rom, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x1FFF + 1, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));

    /* Target 3 boundary check (0x2000 - 0x2FFF) */
    // 0x2FFF + 1 (0x3000) is the start of rom.aliases.alias1, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x2FFF + 1, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));

    /* Target 3 alias1 boundary check (0x3000 - 0x3FFF) */
    // 0x3FFF + 1 (0x4000) is the start of target.aliases.1, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x3FFF + 1, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));

    /* Target 1 alias2 boundary check (0x4000 - 0x4FFF) */
    // 0x4FFF + 1 (0x5000) is the start of m_ram, so it should be OK
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x4FFF + 1, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));

    /* Test for an address truly out of bounds */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x5FFF + 1, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
}

// Debug Transport Interface transaction that crosses the boundary
TEST_BENCH(AliasesMappingTest, SimpleCrossesBoundaryDebug)
{
    /* Target 1 boundary check (0x0000 - 0x0FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x0FFF, 0xFFFF, true),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into alias at 0x1000
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x0FFF - 1, 0xFFFF, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 2);

    /* Target 1 alias1 boundary check (0x1000 - 0x1FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x1FFF, 0xFFFF, true),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into rom at 0x2000
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x1FFF - 1, 0xFFFF, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 2);

    /* Target 3 boundary check (0x2000 - 0x2FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x2FFF, 0xFFFF, true),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into rom alias at 0x3000
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x2FFF - 1, 0xFFFF, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 2);

    /* Target 3 alias1 boundary check (0x3000 - 0x3FFF) */
    ASSERT_EQ(
        m_initiator.do_write<uint16_t>(0x3FFF, 0xFFFF, true),
        tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into target alias2 at 0x4000
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x3FFF - 1, 0xFFFF, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 2);

    /* Target 1 alias2 boundary check (0x4000 - 0x4FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x4FFF, 0xFFFF, true),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should cross into ram at 0x5000
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x4FFF - 1, 0xFFFF, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 2);

    /* Target 2 boundary check (0x5000 - 0x5FFF) */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x5FFF, 0xFFFF, true),
              tlm::TLM_ADDRESS_ERROR_RESPONSE); // Write at end of range, size 2 should be out of bounds
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x5FFF - 1, 0xFFFF, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 2);
}

// Write into memory with Blocking Transport and read with Debug Transport Interface
TEST_BENCH(AliasesMappingTest, WriteBlockingReadDebug)
{
    uint8_t data;

    /* Target 1 main */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0000, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0000, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 1 : alias1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x1000, 0x04), tlm::TLM_OK_RESPONSE); // This is actually target alias1.
    ASSERT_EQ(m_initiator.do_read(0x1000, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 1 : alias2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x4000, 0x04), tlm::TLM_OK_RESPONSE); // This is actually target alias2.
    ASSERT_EQ(m_initiator.do_read(0x4000, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 2 (m_ram) */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x5000, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x5000, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 3 main (m_rom) */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x2000, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x2000, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 3 : alias1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x3000, 0x04), tlm::TLM_OK_RESPONSE); // This is actually target alias1.
    ASSERT_EQ(m_initiator.do_read(0x3000, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);
}

// Write into memory with Debug Transport Interface and read with Blocking Transport
TEST_BENCH(AliasesMappingTest, WriteDebugReadBlocking)
{
    uint8_t data;

    /* Target 1 main */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0000, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 1 : alias1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x1000, 0x04, true),
              tlm::TLM_OK_RESPONSE); // This is actually target alias1.
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x1000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 1 : alias2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x4000, 0x04, true),
              tlm::TLM_OK_RESPONSE); // This is actually target alias2.
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x4000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 2 (m_ram) */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x5000, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x5000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 main (m_rom) */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x2000, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x2000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 : alias1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x3000, 0x04, true),
              tlm::TLM_OK_RESPONSE); // This is actually target alias1.
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x3000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);
}

// Request for DMI access to memory
TEST_BENCH(AliasesMappingTest, SimpleDmi)
{
    /* Valid DMI request Target 1 main */
    do_good_dmi_request_and_check(0x0000, 0x0000, 0x0FFF); // Main target range
    /* Valid DMI request Target 1 : alias1 (should map to m_memory) */
    do_good_dmi_request_and_check(0x1000, 0x1000,
                                  0x1FFF); // Alias address should be reported as the start of the DMI range
    /* Valid DMI request Target 1 : alias2 (should map to m_memory) */
    do_good_dmi_request_and_check(0x4000, 0x4000,
                                  0x4FFF); // Alias address should be reported as the start of the DMI range
    /* Address 0x5000 is the start of m_ram. This is a valid DMI request. */
    do_good_dmi_request_and_check(0x5000, 0x5000, 0x5FFF);
    /* Out-of-bound DMI request Target 2 - truly out of bounds */
    do_bad_dmi_request_and_check(0x5FFF + 1);

    /* Valid DMI request Target 3 main */
    do_good_dmi_request_and_check(0x2000, 0x2000, 0x2FFF); // Main target range
    /* Valid DMI request Target 3 : alias1 (should map to m_rom) */
    do_good_dmi_request_and_check(0x3000, 0x3000,
                                  0x3FFF); // Alias address should be reported as the start of the DMI range
    /* Address 0x4000 is an alias for m_memory. This is a valid DMI request. */
    do_good_dmi_request_and_check(0x4000, 0x4000, 0x4FFF);
}

// Write and Read into the memory with the Direct Memory Interface
TEST_BENCH(AliasesMappingTest, DmiWriteRead)
{
    uint8_t data = 0x04;
    uint8_t data_read;

    // Test addresses for Target 1 (main and aliases)
    std::vector<uint64_t> target1_addrs = { 0x0000, 0x1000, 0x4000 }; // Main, alias1, alias2
    for (auto addr : target1_addrs) {
        uint64_t expected_dmi_start = (addr == 0x0000)
                                          ? 0x0000
                                          : addr; // If it's an alias, expect the alias address as DMI start
        uint64_t expected_dmi_end = expected_dmi_start + 0x0FFF;
        do_good_dmi_request_and_check(addr, expected_dmi_start,
                                      expected_dmi_end); // DMI region should reflect the aliased address
        dmi_write_or_read(addr, &data, sizeof(data), false);
        dmi_write_or_read(addr, &data_read, sizeof(data), true);
        ASSERT_EQ(data, data_read);
    }

    // Test addresses for Target 2 (m_ram)
    std::vector<uint64_t> target2_addrs = { 0x5000 };
    for (auto addr : target2_addrs) {
        do_good_dmi_request_and_check(addr, 0x5000, 0x5FFF);
        dmi_write_or_read(addr, &data, sizeof(data), false);
        dmi_write_or_read(addr, &data_read, sizeof(data), true);
        ASSERT_EQ(data, data_read);
    }

    // Test addresses for Target 3 (main and aliases)
    std::vector<uint64_t> target3_addrs = { 0x2000, 0x3000 }; // Main, alias1
    for (auto addr : target3_addrs) {
        uint64_t expected_dmi_start = (addr == 0x2000)
                                          ? 0x2000
                                          : addr; // If it's an alias, expect the alias address as DMI start
        uint64_t expected_dmi_end = expected_dmi_start + 0x0FFF;
        do_good_dmi_request_and_check(addr, expected_dmi_start,
                                      expected_dmi_end); // DMI region should reflect the aliased address
        dmi_write_or_read(addr, &data, sizeof(data), false);
        dmi_write_or_read(addr, &data_read, sizeof(data), true);
        ASSERT_EQ(data, data_read);
    }
}

int sc_main(int argc, char* argv[])
{
    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig{ "sc_main" };
    auto broker_h = m_broker.create_broker_handle(orig);
    ArgParser ap{ broker_h, argc, argv };

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
