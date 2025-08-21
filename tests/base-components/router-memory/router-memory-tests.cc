/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "router-memory-bench.h"
#include <cci/utils/broker.h>

// Simple read into the memory
TEST_BENCH(RouterMemoryTestBench, SimpleWriteRead)
{
    uint8_t data;
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write(0, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write(address[1], 0x08), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(address[1], data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x08);

    /* Target 3 */
    ASSERT_EQ(m_initiator.do_write(address[3], 0x08), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(address[3], data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x08);
}

// Transaction outside of the target address space
TEST_BENCH(RouterMemoryTestBench, SimpleOverlapWrite)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(memory_size[0], 0x04), tlm::TLM_OK_RESPONSE);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(memory_size[1], 0x08), tlm::TLM_OK_RESPONSE);
}

// Transaction that crosses the boundary
TEST_BENCH(RouterMemoryTestBench, SimpleCrossesBoundary)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(memory_size[0] - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(memory_size[1] - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);
}

// Simple write and read into the memory with the Debug Transport Interface
TEST_BENCH(RouterMemoryTestBench, SimpleWriteReadDebug)
{
    uint8_t data8 = 0;
    uint16_t data16 = 0;
    uint32_t data32 = 0;
    uint64_t data64 = 0;

    // set some data so that we are sure we only get the right amount below
    ASSERT_EQ(m_initiator.do_write<uint64_t>(0, -1, true), tlm ::TLM_OK_RESPONSE);

    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data8, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data8));
    ASSERT_EQ(data8, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint16_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint32_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint32_t));
    ASSERT_EQ(m_initiator.do_read(0, data32, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data32));
    ASSERT_EQ(data32, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint64_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint64_t));
    ASSERT_EQ(m_initiator.do_read(0, data64, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data64));
    ASSERT_EQ(data64, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(address[1], 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data8, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data8));
    ASSERT_EQ(data8, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint16_t>(address[1], 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint32_t>(address[1], 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint32_t));
    ASSERT_EQ(m_initiator.do_read(0, data32, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data32));
    ASSERT_EQ(data32, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint64_t>(address[1], 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint64_t));
    ASSERT_EQ(m_initiator.do_read(0, data64, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data64));
    ASSERT_EQ(data64, 0x04);

    ASSERT_EQ(m_initiator.do_write(0x10000, data32), tlm::TLM_OK_RESPONSE);
}

// Debug Transport Interface transaction outside of the target address space
TEST_BENCH(RouterMemoryTestBench, SimpleOverlapWriteDebug)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(memory_size[0], 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_NE(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(memory_size[1], 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_NE(m_initiator.get_last_transport_debug_ret(), 0);
}

// Debug Transport Interface transaction that crosses the boundary
TEST_BENCH(RouterMemoryTestBench, SimpleCrossesBoundaryDebug)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(memory_size[0] - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(memory_size[1] - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
}

// Write into memory with Blocking Transport and read with Debug Transport Interface
TEST_BENCH(RouterMemoryTestBench, WriteBlockingReadDebug)
{
    uint8_t data;

    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(address[1], 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(address[1], data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    gs::memorydumper_tgr_helper();
}

// Write into memory with Debug Transport Interface and read with Blocking Transport
TEST_BENCH(RouterMemoryTestBench, WriteDebugReadBlocking)
{
    uint8_t data;

    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(address[1], 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(address[1], data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);
}

// Request for DMI access to memory
TEST_BENCH(RouterMemoryTestBench, SimpleDmi)
{
    /* Valid DMI request Target 0 (address[0]=0x0) */
    do_good_dmi_request_and_check(address[0], address[0], memory_size[0] - 1);

    /* Address 0x1000 (memory_size[0]) falls into Memory 1 (starts at 0x200) */
    do_good_dmi_request_and_check(memory_size[0], address[1], memory_size[1] - 1);

    /* Address 0x200 (address[1]) falls into Memory 0 (starts at 0x0) */
    do_good_dmi_request_and_check(address[1], address[0], memory_size[0] - 1);

    /* Address 0x1200 (memory_size[1]) falls into Memory 2 (starts at 0x400) */
    do_good_dmi_request_and_check(memory_size[1], address[2], memory_size[2] - 1);

    // Add a check for a truly out-of-bounds address, e.g., beyond the last memory
    // Max address in current config is 0x600 + 0x1000 - 1 = 0x15ff
    do_bad_dmi_request_and_check(0x2000);
}

// Write and Read into the mémory with the Direct Memory Interface
TEST_BENCH(RouterMemoryTestBench, DmiWriteRead)
{
    uint8_t data = 0x04;
    uint8_t data_read;

    /* Valid DMI request target N°0 (address[0]+0x50 = 0x50) */
    do_good_dmi_request_and_check(address[0] + 0x50, address[0], memory_size[0] - 1);
    /* Write with DMI target N°0 */
    dmi_write_or_read(0x50, &data, sizeof(data), false);
    /* Read with DMI target N°0 */
    dmi_write_or_read(0x50, &data_read, sizeof(data), true);
    ASSERT_EQ(data, data_read);

    /* Valid DMI request target N°0 (address[1]+0x50 = 0x250 which falls in Memory 0) */
    do_good_dmi_request_and_check(address[1] + 0x50, address[0], memory_size[0] - 1);
    /* Write with DMI target N°0 */
    dmi_write_or_read(address[1] + 0x50, &data, sizeof(data), false);
    /* Read with DMI target N°0 */
    dmi_write_or_read(address[1] + 0x50, &data_read, sizeof(data), true);
    ASSERT_EQ(data, data_read);
}

int sc_main(int argc, char* argv[])
{
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
