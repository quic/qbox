/*
* Copyright (c) 2022 GreenSocs
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version, or under the
* Apache License, Version 2.0 (the "License”) at your discretion.
*
* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
* You may obtain a copy of the Apache License at
* http://www.apache.org/licenses/LICENSE-2.0
*/

#include "memory-bench.h"
#include <cci/utils/broker.h>

// Simple read into the memory
TEST_BENCH(MemoryTestBench, SimpleWriteRead)
{
    uint8_t data;
    ASSERT_EQ(m_initiator.do_write(0, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);
}

// Transaction outside of the target address space
TEST_BENCH(MemoryTestBench, SimpleOverlapWrite)
{
    ASSERT_EQ(m_initiator.do_write<uint8_t>(MEMORY_SIZE + 1, 0x04), tlm::TLM_ADDRESS_ERROR_RESPONSE);
}

// Transaction that crosses the boundary
TEST_BENCH(MemoryTestBench, SimpleCrossesBoundary)
{
    ASSERT_EQ(m_initiator.do_write<uint16_t>(MEMORY_SIZE - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);
}

// Simple write and read into the memory with the Debug Transport Interface
TEST_BENCH(MemoryTestBench, SimpleWriteReadDebug)
{
    uint64_t data;

    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint16_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0, data, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint32_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint32_t));
    ASSERT_EQ(m_initiator.do_read(0, data, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint64_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint64_t));
    ASSERT_EQ(m_initiator.do_read(0, data, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);
}

// Debug Transport Interface transaction outside of the target address space
TEST_BENCH(MemoryTestBench, SimpleOverlapWriteDebug)
{
    ASSERT_EQ(m_initiator.do_write<uint8_t>(MEMORY_SIZE + 1, 0x04, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
}

// Debug Transport Interface transaction that crosses the boundary
TEST_BENCH(MemoryTestBench, SimpleCrossesBoundaryDebug)
{
    ASSERT_EQ(m_initiator.do_write<uint16_t>(MEMORY_SIZE - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
}

// Write into memory with Blocking Transport and read with Debug Transport Interface
TEST_BENCH(MemoryTestBench, WriteBlockingReadDebug)
{
    uint8_t data;
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);
}

// Write into memory with Debug Transport Interface and read with Blocking Transport
TEST_BENCH(MemoryTestBench, WriteDebugReadBlocking)
{
    uint8_t data;
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);
}

// Request for DMI access to memory
TEST_BENCH(MemoryTestBench, SimpleDmi)
{

    /* Valid DMI request */
    do_good_dmi_request_and_check(0, 0, MEMORY_SIZE - 1);

    /* Out-of-bound DMI request */
    do_bad_dmi_request_and_check(MEMORY_SIZE);
}

// Write and Read into the mémory with the Direct Memory Interface
TEST_BENCH(MemoryTestBench, DmiWriteRead)
{
    uint8_t data = 0x04;
    uint8_t data_read;

    /* Valid DMI request */
    do_good_dmi_request_and_check(0, 0, MEMORY_SIZE - 1);

    /* Write with DMI */
    dmi_write_or_read(0, data, sizeof(data), false);

    /* Read with DMI */
    dmi_write_or_read(0, data_read, sizeof(data), true);
    ASSERT_EQ(data, data_read);
}

int sc_main(int argc, char* argv[])
{
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}