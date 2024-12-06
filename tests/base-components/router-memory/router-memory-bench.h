/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "gs_memory.h"
#include "router.h"
#include "memory_dumper.h"
#include <tests/initiator-tester.h>
#include <tests/test-bench.h>

static constexpr size_t NB_MEMORY = 4;

class RouterMemoryTestBench : public TestBench
{
public:
    std::vector<uint64_t> address = { 0, 257, 524, 700 };
    std::vector<size_t> size = { 256, 256, 256, 256 };
    std::vector<uint64_t> memory_size;

protected:
    InitiatorTester m_initiator;
    gs::router<> m_router;
    std::vector<gs::gs_memory<>*> m_memory;
    gs::memory_dumper<> m_dumper;

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
    RouterMemoryTestBench(const sc_core::sc_module_name& n)
        : TestBench(n), m_initiator("initiator"), m_router("router"), m_memory(), m_dumper("dumper")
    {
        for (int i = 0; i < NB_MEMORY; i++) {
            char txt[20];
            snprintf(txt, 20, "Memory_%d", i);
            m_memory.push_back(new gs::gs_memory<>(txt, size[i]));
            memory_size.push_back(address[i] + size[i]);
        }

        m_initiator.register_invalidate_direct_mem_ptr(
            [this](uint64_t start, uint64_t end) { invalidate_direct_mem_ptr(start, end); });

        m_router.add_initiator(m_initiator.socket);
        for (int i = 0; i < NB_MEMORY; i++) {
            m_router.add_target(m_memory[i]->socket, address[i], size[i]);
        }

        m_router.add_initiator(m_dumper.initiator_socket);
        m_router.add_target(m_dumper.target_socket, 0x10000, 0x10);
    }

    virtual ~RouterMemoryTestBench()
    {
        while (!m_memory.empty()) {
            delete m_memory.back();
            m_memory.pop_back();
        }
    }
};
