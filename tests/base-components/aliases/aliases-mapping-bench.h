/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <cci_configuration>

#include <luafile_tool.h>
#include <libgsutils.h>
#include <argparser.h>

#include "memory.h"
#include "router.h"
#include <tests/initiator-tester.h>
#include <tests/test-bench.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class AliasesMappingTest : public TestBench
{
protected:
    InitiatorTester m_initiator;
    gs::router<> m_router;
    gs::memory<> m_memory;
    gs::memory<> m_ram;
    gs::memory<> m_rom;

    void do_good_dmi_request_and_check(uint64_t addr, int64_t exp_start, uint64_t exp_end)
    {
        using namespace tlm;

        bool ret = m_initiator.do_dmi_request(addr);
        const tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();

        ASSERT_TRUE(ret);
        ASSERT_EQ(exp_start, dmi_data.get_start_address());
        ASSERT_EQ(exp_end, dmi_data.get_end_address());
    }

    void do_bad_dmi_request_and_check(uint64_t addr)
    {
        using namespace tlm;

        bool ret = m_initiator.do_dmi_request(addr);

        ASSERT_FALSE(ret);
    }

    void dmi_write_or_read(uint64_t addr, uint8_t* data, size_t len, bool is_read)
    {
        using namespace tlm;

        const tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();
        addr -= dmi_data.get_start_address();

        if (is_read) {
            ASSERT_TRUE(dmi_data.is_read_allowed());
            memcpy(data, dmi_data.get_dmi_ptr() + addr, len);
        } else {
            ASSERT_TRUE(dmi_data.is_write_allowed());
            memcpy(dmi_data.get_dmi_ptr() + addr, data, len);
        }
    }

    void do_bus_binding()
    {
        m_router.initiator_socket.bind(m_memory.socket);
        m_router.initiator_socket.bind(m_ram.socket);
        m_router.initiator_socket.bind(m_rom.socket);
        m_router.add_initiator(m_initiator.socket);
    }

public:
    AliasesMappingTest(const sc_core::sc_module_name& n)
        : TestBench(n), m_initiator("initiator"), m_router("router"), m_memory("target"), m_ram("ram"), m_rom("rom")
    {
        do_bus_binding();
    }
};
