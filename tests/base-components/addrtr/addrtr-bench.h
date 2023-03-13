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

#ifndef _BASE_COMPONENTS_TESTS_ADDRTR_TEST_BENCH_H
#define _BASE_COMPONENTS_TESTS_ADDRTR_TEST_BENCH_H

#include <functional>

#include <systemc>
#include <tlm>

#include <greensocs/gsutils/tests/test-bench.h>
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/target-tester.h>

#include "greensocs/base-components/misc/addrtr.h"

class AddrtrTestBench : public TestBench
{
    static constexpr size_t TARGET_MMIO_SIZE = 1024;

    class TargetTesterDMIinv : public TargetTester
    {
        using TargetTester::TargetTester;

    public:
        void do_dmi_invalidate(uint64_t addr)
        {
            socket->invalidate_direct_mem_ptr(addr, addr + TARGET_MMIO_SIZE);
        }
    };

public:
    using TlmResponseStatus = InitiatorTester::TlmResponseStatus;
    using TlmGenericPayload = InitiatorTester::TlmGenericPayload;
    using TlmDmi = InitiatorTester::TlmDmi;

private:
    Addrtr m_addrtr;

    InitiatorTester m_initiator;
    TargetTesterDMIinv m_target;

    uint64_t sent_addr;
    uint64_t offset = 0xDEAD;

    /* Initiator callback */
    void invalidate_direct_mem_ptr(uint64_t start_range, uint64_t end_range)
    {
        EXPECT_EQ(start_range, sent_addr);
        EXPECT_EQ(end_range, sent_addr + TARGET_MMIO_SIZE);
    }

    /* Target callbacks */
    TlmResponseStatus target_access(uint64_t addr, uint8_t* data, size_t len)
    {
        EXPECT_EQ(sent_addr + offset, addr);
        return tlm::TLM_OK_RESPONSE;
    }

    bool get_direct_mem_ptr(uint64_t addr, TlmDmi& dmi_data)
    {
        EXPECT_EQ(addr, sent_addr + offset);
        return true;
    }

protected:
    void do_txn(int id, uint64_t addr, bool dbg)
    {
        uint64_t data = 0x42;
        TlmGenericPayload txn;
        sent_addr = addr;
        if (!dbg) {
            ASSERT_EQ(tlm::TLM_OK_RESPONSE, m_initiator.do_write(addr, data, dbg));
        } else {
            ASSERT_EQ(0, m_initiator.do_write(addr, data, dbg));
        }
    }

    void do_dmi(int id, uint64_t addr)
    {
        uint64_t data = 0x42;
        TlmGenericPayload txn;
        sent_addr = addr;
        m_initiator.do_dmi_request(addr);
        m_target.do_dmi_invalidate(addr + offset);
    }

public:
    AddrtrTestBench(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_addrtr("exclusive-addrtr")
        , m_initiator("initiator-tester")
        , m_target("target-tester", TARGET_MMIO_SIZE)
    {
        using namespace std::placeholders;

        offset = 10;
        m_addrtr.offset = offset;

        m_initiator.register_invalidate_direct_mem_ptr(
            std::bind(&AddrtrTestBench::invalidate_direct_mem_ptr, this, _1, _2));
        m_target.register_read_cb(std::bind(&AddrtrTestBench::target_access, this, _1, _2, _3));
        m_target.register_write_cb(std::bind(&AddrtrTestBench::target_access, this, _1, _2, _3));
        m_target.register_debug_write_cb(
            std::bind(&AddrtrTestBench::target_access, this, _1, _2, _3));
        m_target.register_get_direct_mem_ptr_cb(
            std::bind(&AddrtrTestBench::get_direct_mem_ptr, this, _1, _2));

        m_addrtr.front_socket.bind(m_initiator.socket);
        m_addrtr.back_socket.bind(m_target.socket);
    }

    virtual ~AddrtrTestBench() {}
};

#endif