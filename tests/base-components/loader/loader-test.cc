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

#include "loader-test-bench.h"

// Simple read into the memory and write with elf file
TEST_BENCH(LoaderTest, SimpleReadELFFile)
{
    uint8_t data;
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_read(0x0003, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xde);

    ASSERT_EQ(m_initiator.do_read(0x0002, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xad);

    ASSERT_EQ(m_initiator.do_read(0x0001, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xbe);

    ASSERT_EQ(m_initiator.do_read(0x0000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xaf);
}

TEST_BENCH(LoaderTest, SimpleReadData)
{
    uint8_t data;
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_read(0x0503, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xde);

    ASSERT_EQ(m_initiator.do_read(0x0502, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xad);

    ASSERT_EQ(m_initiator.do_read(0x0501, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xbe);

    ASSERT_EQ(m_initiator.do_read(0x0000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xaf);
}

TEST_BENCH(LoaderTest, SimpleReadBinFile)
{
    uint8_t data;
    /* Target 2 */
    ASSERT_EQ(m_initiator.do_read(0x1003, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xde);

    ASSERT_EQ(m_initiator.do_read(0x1002, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xad);

    ASSERT_EQ(m_initiator.do_read(0x1001, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xbe);

    ASSERT_EQ(m_initiator.do_read(0x1000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xaf);
}

// Simple read into the memory and write with csv file
TEST_BENCH(LoaderTest, SimpleReadCSVFile)
{
    uint32_t data;
    /* Target 3 */
    ASSERT_EQ(m_initiator.do_read(0x2000, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 11);

    ASSERT_EQ(m_initiator.do_read(0x2004, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 12);

    ASSERT_EQ(m_initiator.do_read(0x2008, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 15);

    uint64_t data64;
    ASSERT_EQ(m_initiator.do_read(0x200c, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0xafafb5b5);
    ASSERT_EQ(m_initiator.do_read(0x200c, data64), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data64, 0xf00afafb5b5);
}

int sc_main(int argc, char* argv[])
{
    auto m_broker = new gs::ConfigurableBroker(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
