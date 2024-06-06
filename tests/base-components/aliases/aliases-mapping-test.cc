/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "aliases-mapping-bench.h"

// Simple read into the memory
TEST_BENCH(AliasesMappingTest, SimpleWriteRead)
{
    uint8_t data;
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write(0x0000, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 1 : first aliases */
    ASSERT_EQ(m_initiator.do_write(0X0100, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0X0100, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 1 : second aliases */
    ASSERT_EQ(m_initiator.do_write(0x0200, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0200, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 */
    ASSERT_EQ(m_initiator.do_write(0x0400, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0400, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 : first aliases */
    ASSERT_EQ(m_initiator.do_write(0X0500, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0X0500, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 : second aliases */
    ASSERT_EQ(m_initiator.do_write(0x0600, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0600, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);
}

// Transaction outside of the target address space
TEST_BENCH(AliasesMappingTest, SimpleOverlapWrite)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x00FF, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 1 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x01FF, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 1 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x02FF, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x03FF, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 3 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x04FF, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 3 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x05FF, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 3 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x06FF, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);
}

// Transaction that crosses the boundary
TEST_BENCH(AliasesMappingTest, SimpleCrossesBoundary)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x00FF - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 1 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x01FF - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 1 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x02FF - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x03FF - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 3 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x04FF - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 3 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x05FF - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 3 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x06FF - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);
}

TEST_BENCH(AliasesMappingTest, SimpleWriteReadDebug)
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

    /* Target 1 : first aliases*/
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x0100, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);

    /* Target 1 : second aliases*/
    ASSERT_EQ(m_initiator.do_write<uint32_t>(0x0200, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint32_t));
    ASSERT_EQ(m_initiator.do_read(0x0200, data32, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data32));
    ASSERT_EQ(data32, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint64_t>(0x0300, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint64_t));
    ASSERT_EQ(m_initiator.do_read(0x0300, data64, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data64));
    ASSERT_EQ(data64, 0x04);

    /* Target 3 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0400, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0400, data8, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data8));
    ASSERT_EQ(data8, 0x04);

    /* Target 3 : first aliases*/
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x0500, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0x0500, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);

    /* Target 3 : second aliases*/
    ASSERT_EQ(m_initiator.do_write<uint32_t>(0x0600, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint32_t));
    ASSERT_EQ(m_initiator.do_read(0x0600, data32, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data32));
    ASSERT_EQ(data32, 0x04);
}

// Debug Transport Interface transaction outside of the target address space
TEST_BENCH(AliasesMappingTest, SimpleOverlapWriteDebug)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x00FF, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 1 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x01FF, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 1 : second aliases*/
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x02FF, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x03FF, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 3 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x04FF, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 3 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x05FF, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 3 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x06FF, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
}

// Debug Transport Interface transaction that crosses the boundary
TEST_BENCH(AliasesMappingTest, SimpleCrossesBoundaryDebug)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x00FF - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x01FF - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x02FF - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x03FF - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x04FF - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x05FF - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(0x06FF - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
}

// Write into memory with Blocking Transport and read with Debug Transport Interface
TEST_BENCH(AliasesMappingTest, WriteBlockingReadDebug)
{
    uint8_t data;

    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 1 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0100, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0100, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 1 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0200, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0200, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0300, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0300, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 3 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0400, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0400, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 3 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0500, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0500, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 3 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0600, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0x0600, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);
}

// Write into memory with Debug Transport Interface and read with Blocking Transport
TEST_BENCH(AliasesMappingTest, WriteDebugReadBlocking)
{
    uint8_t data;

    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 1 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0100, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0100, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 1 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0200, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0200, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0300, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0300, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 : */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0400, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0400, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 : first aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0500, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0500, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 3 : second aliases */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0x0600, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0x0600, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);
}

// Request for DMI access to memory
TEST_BENCH(AliasesMappingTest, SimpleDmi)
{
    /* Valid DMI request Target 1 */
    do_good_dmi_request_and_check(0, 0, 0x00FF - 1);
    /* Out-of-bound DMI request Target 1 */
    do_bad_dmi_request_and_check(0x00FF);

    /* Valid DMI request Target 1 : first aliases */
    do_good_dmi_request_and_check(0x0100, 0x0100, 0x01FF - 1);
    /* Out-of-bound DMI request Target 1 : first aliases */
    do_bad_dmi_request_and_check(0x01FF);

    // /* Valid DMI request Target 1 : second aliases */
    do_good_dmi_request_and_check(0x0200, 0x0200, 0x02FF - 1);
    // /* Out-of-bound DMI request Target 1 : second aliases */
    do_bad_dmi_request_and_check(0x02FF);

    // /* Valid DMI request Target 2 */
    do_good_dmi_request_and_check(0x0300, 0x0300, 0x03FF - 1);
    // /* Out-of-bound DMI request Target 2 */
    do_bad_dmi_request_and_check(0x03FF);

    // /* Valid DMI request Target 3 */
    do_good_dmi_request_and_check(0x0400, 0x0400, 0x04FF - 1);
    // /* Out-of-bound DMI request Target 3 */
    do_bad_dmi_request_and_check(0x04FF);

    // /* Valid DMI request Target 3 : first aliases */
    do_good_dmi_request_and_check(0x0500, 0x0500, 0x05FF - 1);
    // /* Out-of-bound DMI request Target 3 : first aliases */
    do_bad_dmi_request_and_check(0x05FF);

    // /* Valid DMI request Target 3 : second aliases */
    do_good_dmi_request_and_check(0x0600, 0x0600, 0x06FF - 1);
    // /* Out-of-bound DMI request Target 3 : second aliases */
    do_bad_dmi_request_and_check(0x06FF);
}

// Write and Read into the mémory with the Direct Memory Interface
TEST_BENCH(AliasesMappingTest, DmiWriteRead)
{
    uint8_t data = 0x04;
    uint8_t data_read;

    std::vector<uint64_t> addrs = { 0x0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600 };
    for (auto addr : addrs) {
        do_good_dmi_request_and_check(addr, addr, addr + 0xFF - 1);
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