/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <vector>
#include <algorithm>

#include <tlm>
#include <scp/report.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <gtest/gtest.h>

#include "gs_memory.h"
#include "loader.h"
#include <cciutils.h>
#include <tests/initiator-tester.h>

class MemoryBlocs : public sc_core::sc_module
{
public:
    void test_write_one_bloc()
    {
        constexpr size_t block_size = 0x200;
        std::vector<int> tab(block_size, 4);
        std::vector<int> data1(block_size, 0);

        m_initiator.do_write_with_ptr(0x450, reinterpret_cast<const uint8_t*>(tab.data()), tab.size() * sizeof(int));
        m_initiator.do_read_with_ptr(0x450, reinterpret_cast<uint8_t*>(data1.data()), data1.size() * sizeof(int));
        ASSERT_EQ(data1, tab);

        uint8_t data_8t = 0;
        m_initiator.do_write<uint8_t>(0x10000, 0xFF);
        m_initiator.do_read(0x10000, data_8t);
        ASSERT_TRUE(0xFF == data_8t);

        uint16_t data_16t = 0;
        m_initiator.do_write<uint16_t>(0x20000, 0xABCD);
        m_initiator.do_read(0x20000, data_16t);
        ASSERT_TRUE(0xABCD == data_16t);

        uint32_t data_32t = 0;
        m_initiator.do_write<uint32_t>(0x30000, 0xABCDEF);
        m_initiator.do_read(0x30000, data_32t);
        ASSERT_TRUE(0xABCDEF == data_32t);

        uint64_t data_64t = 0;
        m_initiator.do_write<uint64_t>(0x50000, 0xABCDEFAF);
        m_initiator.do_read(0x50000, data_64t);
        ASSERT_TRUE(0xABCDEFAF == data_64t);
    }

    void test_write_two_blocs()
    {
        constexpr size_t block_size = 0x80000;
        const std::vector<int> tab(block_size, 4);
        std::vector<int> data1(block_size, 0);

        m_initiator.do_write_with_ptr(0x33FFFFF00, reinterpret_cast<const uint8_t*>(tab.data()),
                                      tab.size() * sizeof(int));
        m_initiator.do_read_with_ptr(0x33FFFFF00, reinterpret_cast<uint8_t*>(data1.data()), data1.size() * sizeof(int));

        ASSERT_EQ(data1, tab);

        uint8_t data = 0;
        m_initiator.do_write<uint8_t>(0x400000000, 0x66);
        m_initiator.do_read(0x400000000, data);
        ASSERT_TRUE(0x66 == data);
    }

    void test_write_debug_one_bloc()
    {
        constexpr size_t block_size = 0x200;
        std::vector<int> tab(block_size, 4);
        std::vector<int> data1(block_size, 0);

        m_initiator.do_write_with_ptr(0x100000, reinterpret_cast<const uint8_t*>(tab.data()), tab.size() * sizeof(int),
                                      true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == tab.size() * sizeof(int));
        m_initiator.do_read_with_ptr(0x100000, reinterpret_cast<uint8_t*>(data1.data()), data1.size() * sizeof(int),
                                     true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == data1.size() * sizeof(int));

        ASSERT_EQ(data1, tab);

        uint8_t data_8t = 0;
        m_initiator.do_write<uint8_t>(0x200000, 0x90, true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == sizeof(uint8_t));
        m_initiator.do_read(0x200000, data_8t, true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == sizeof(data_8t));
        ASSERT_TRUE(0x90 == data_8t);

        uint16_t data_16t = 0;
        m_initiator.do_write<uint16_t>(0x300000, 0xABCD, true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == sizeof(uint16_t));
        m_initiator.do_read(0x300000, data_16t, true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == sizeof(data_16t));
        ASSERT_TRUE(0xABCD == data_16t);

        uint32_t data_32t = 0;
        m_initiator.do_write<uint32_t>(0x400000, 0xFFABEF, true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == sizeof(uint32_t));
        m_initiator.do_read(0x400000, data_32t, true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == sizeof(data_32t));
        ASSERT_TRUE(0xFFABEF == data_32t);

        uint64_t data_64t = 0;
        m_initiator.do_write<uint64_t>(0x500000, 0x12345678, true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == sizeof(uint64_t));
        m_initiator.do_read(0x500000, data_64t, true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == sizeof(data_64t));
        ASSERT_TRUE(0x12345678 == data_64t);
    }

    void test_write_debug_two_blocs()
    {
        constexpr size_t block_size = 0x200;
        std::vector<int> tab(block_size, 4);
        std::vector<int> data(block_size, 0);

        m_initiator.do_write_with_ptr(0x33FFFFF00, reinterpret_cast<const uint8_t*>(tab.data()),
                                      tab.size() * sizeof(int), true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == tab.size() * sizeof(int));
        m_initiator.do_read_with_ptr(0x33FFFFF00, reinterpret_cast<uint8_t*>(data.data()), data.size() * sizeof(int),
                                     true);
        ASSERT_TRUE(m_initiator.get_last_transport_debug_ret() == data.size() * sizeof(int));

        ASSERT_EQ(data, tab);
    }

    void do_good_dmi_request_and_check(uint64_t addr)
    {
        using namespace tlm;

        bool ret = m_initiator.do_dmi_request(addr);

        ASSERT_TRUE(ret);
    }

    void dmi_write_or_read(uint64_t addr, uint8_t* data, size_t data_len, bool is_read)
    {
        using namespace tlm;

        uint64_t len = 0;
        uint64_t data_ptr_index = 0;

        while (data_len > 0) {
            do_good_dmi_request_and_check(addr);
            const tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();

            uint64_t start_addr = dmi_data.get_start_address();
            uint64_t end_addr = dmi_data.get_end_address();
            uint64_t bloc_size = (end_addr - start_addr) + 1;

            uint64_t bloc_offset = addr - start_addr;
            uint64_t bloc_len = bloc_size - bloc_offset;
            len = (data_len < bloc_len) ? data_len : bloc_len;

            if (is_read == TLM_READ_COMMAND) {
                ASSERT_TRUE(dmi_data.is_read_allowed());
                memcpy(&data[data_ptr_index], dmi_data.get_dmi_ptr() + bloc_offset, len);
            } else {
                ASSERT_TRUE(dmi_data.is_write_allowed());
                memcpy(dmi_data.get_dmi_ptr() + bloc_offset, &data[data_ptr_index], len);
            }

            data_ptr_index += len;
            data_len -= len;
            addr += len;
        }
    }

    void test_write_with_DMI()
    {
        uint8_t data = 0xFF;
        std::vector<uint8_t> tab(0x80000, 8);
        std::vector<uint8_t> data1(0x80000, 0);
        uint8_t data_read = 0;

        /* Simple send/read data block 0*/
        dmi_write_or_read(0x100, &data, sizeof(data), true);
        dmi_write_or_read(0x100, &data_read, sizeof(data), false);
        ASSERT_TRUE(data == data_read);

        data = 0x04;
        /* Simple send/read data block 1 */
        dmi_write_or_read(0x400000000, &data, sizeof(data), true);
        dmi_write_or_read(0x400000000, &data_read, sizeof(data), false);
        ASSERT_TRUE(data == data_read);

        dmi_write_or_read(0x33FFFFF00, tab.data(), tab.size(), true);
        dmi_write_or_read(0x33FFFFF00, data1.data(), data1.size(), false);

        ASSERT_EQ(data1, tab);
    }

    template <class T>
    void test_rw()
    {
        T seed = 0x11;
        for (int i = 0; i < sizeof(T); i++) seed += seed << 8;
        for (int i = 1; i < 30; i++) {
            for (int a = 0; a < 16; a++) {
                uint64_t addr = ((i * 0x1000) - (8 * sizeof(T))) + (a * sizeof(T));
                ASSERT_TRUE(m_initiator.do_write(addr, seed * a, false) == tlm::TLM_OK_RESPONSE);
            }
        }
        for (int i = 1; i < 3; i++) {
            for (int a = 0; a < 16; a++) {
                T d;
                uint64_t addr = ((i * 0x1000) - (8 * sizeof(T))) + (a * sizeof(T));
                ASSERT_TRUE(m_initiator.do_read(addr, d, false) == tlm::TLM_OK_RESPONSE);
                ASSERT_TRUE(d == (seed * a));
            }
        }
    }

    template <class T>
    void test_rdmiw()
    {
        T seed = 0x11;
        uint8_t* old_ptr;
        int boundry = 0;
        for (int i = 0; i < sizeof(T); i++) seed += seed << 8; // make a fairly random data thats different in each byte
        for (int i = 1; i < 30; i++) {
            old_ptr = 0;
            for (int a = 0; a < 16; a++) {
                uint64_t addr = ((i * 0x1000) - (8 * sizeof(T))) + (a * sizeof(T));

                do_good_dmi_request_and_check(addr);
                const tlm::tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();
                uint64_t start_addr = dmi_data.get_start_address();
                uint64_t end_addr = dmi_data.get_end_address();
                uint8_t* ptr = dmi_data.get_dmi_ptr();
                if (old_ptr != ptr) {
                    ASSERT_TRUE(a == 0 || a == 8);
                    if (a == 8) {
                        boundry++;
                    }
                }
                old_ptr = ptr;
                ASSERT_TRUE(start_addr <= addr);
                ASSERT_TRUE(addr <= end_addr);
                T data = seed * a;
                memcpy(dmi_data.get_dmi_ptr() + (addr - start_addr), &data, sizeof(T));
            }
        }
        for (int i = 1; i < 3; i++) {
            for (int a = 0; a < 16; a++) {
                T d;
                uint64_t addr = ((i * 0x1000) - (8 * sizeof(T))) + (a * sizeof(T));
                ASSERT_TRUE(m_initiator.do_read(addr, d, false) == tlm::TLM_OK_RESPONSE);
                ASSERT_TRUE(d == (seed * a));
            }
        }
        ASSERT_TRUE(boundry > 0);
    }

protected:
    gs::ConfigurableBroker m_broker;
    InitiatorTester m_initiator;
    gs::gs_memory<> m_memory;

public:
    MemoryBlocs(const sc_core::sc_module_name& n)
        : sc_core::sc_module(n), m_broker(), m_initiator("initiator"), m_memory("memory")
    {
        m_initiator.socket.bind(m_memory.socket);
    }

    virtual ~MemoryBlocs() {}
};

int sc_main(int argc, char* argv[])
{
    static constexpr size_t MEMORY_SIZE = 0xD00000000;

    auto m_broker = new gs::ConfigurableBroker({
        { "test.memory.min_block_size", cci::cci_value(0x1000) },
        { "test.memory.max_block_size", cci::cci_value(0x10000) },
        { "test.memory.target_socket.size", cci::cci_value(MEMORY_SIZE) },
        { "test.memory.target_socket.address", cci::cci_value(0) },
        { "test.memory.target_socket.relative_addresses", cci::cci_value(true) },
    });

    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(10));      // make the msg type column a bit tighter

    MemoryBlocs test("test");

    sc_core::sc_start();

    test.test_write_one_bloc();
    test.test_write_two_blocs();
    test.test_write_debug_one_bloc();
    test.test_write_debug_two_blocs();
    test.test_write_with_DMI();

    test.test_rdmiw<uint8_t>();
    test.test_rdmiw<uint64_t>();
    test.test_rw<uint8_t>();
    test.test_rw<uint64_t>();
    return 0;
}