
/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "memory.h"
#include "loader.h"
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>

class MemoryTestBench : public TestBench
{
public:
    static constexpr size_t MEMORY_SIZE = 256;

protected:
    InitiatorTester m_initiator;
    gs::memory<> m_target;

    /* Initiator callback */
    void invalidate_direct_mem_ptr(uint64_t start_range, uint64_t end_range)
    {
        ADD_FAILURE(); /* we don't expect any invalidation */
    }

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

public:
    MemoryTestBench(const sc_core::sc_module_name& n)
        : TestBench(n), m_initiator("initiator"), m_target("memory", MEMORY_SIZE)
    {
        m_initiator.register_invalidate_direct_mem_ptr(
            [this](uint64_t start, uint64_t end) { invalidate_direct_mem_ptr(start, end); });

        m_initiator.socket.bind(m_target.socket);
    }

    virtual ~MemoryTestBench() {}
};
