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

#ifndef _BASE_COMPONENTS_TESTS_EXCLUSIVE_MONITOR_TEST_BENCH_H
#define _BASE_COMPONENTS_TESTS_EXCLUSIVE_MONITOR_TEST_BENCH_H

#include <functional>

#include <systemc>
#include <tlm>

#include <greensocs/gsutils/tests/test-bench.h>
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/target-tester.h>

#include "greensocs/base-components/misc/exclusive-monitor.h"
#include <greensocs/base-components/router.h>

class ExclusiveMonitorTestBench : public TestBench {
public:
    static constexpr uint64_t TARGET_MMIO_SIZE = 1024;

    using TlmResponseStatus = InitiatorTester::TlmResponseStatus;
    using TlmGenericPayload = InitiatorTester::TlmGenericPayload;
    using TlmDmi = InitiatorTester::TlmDmi;

private:
    ExclusiveMonitor m_monitor;

    sc_core::sc_vector<InitiatorTester> m_initiators;
    InitiatorTester m_initiator;
    TargetTester m_target;
    gs::Router<> m_router;

    bool m_last_dmi_inval_valid = false;
    uint64_t m_last_dmi_inval_start;
    uint64_t m_last_dmi_inval_end;

    /* Initiator callback */
    void invalidate_direct_mem_ptr(uint64_t start_range, uint64_t end_range)
    {
        m_last_dmi_inval_valid = true;
        m_last_dmi_inval_start = start_range;
        m_last_dmi_inval_end = end_range;
    }

    /* Target callbacks */
    TlmResponseStatus target_access(uint64_t addr, uint8_t *data, size_t len)
    {
        m_target.get_cur_txn().set_dmi_allowed(true);
        return tlm::TLM_OK_RESPONSE;
    }

    bool get_direct_mem_ptr(uint64_t addr, TlmDmi &dmi_data)
    {
        if (addr >= TARGET_MMIO_SIZE) {
            return false;
        } else {
            dmi_data.allow_read_write();
            dmi_data.set_dmi_ptr(nullptr);
            dmi_data.set_start_address(0);
            dmi_data.set_end_address(TARGET_MMIO_SIZE - 1);

            return true;
        }
    }


    void check_txn(bool is_load, uint64_t addr, size_t len, TlmResponseStatus ret)
    {
        using namespace tlm;

        tlm_command cmd = is_load ? TLM_READ_COMMAND : TLM_WRITE_COMMAND;

        if (ret == TLM_GENERIC_ERROR_RESPONSE) {
            /*
             * The monitor denied the exclusive store, the target should not
             * have received the transaction.
             */
            ASSERT_FALSE(m_target.last_txn_is_valid());
            return;
        } else if (addr + len >= TARGET_MMIO_SIZE) {
            ASSERT_EQ(ret, TLM_ADDRESS_ERROR_RESPONSE);
        }

        ASSERT_TRUE(m_target.last_txn_is_valid());
        const TlmGenericPayload &target_txn = m_target.get_last_txn();

        ASSERT_EQ(target_txn.get_address(), addr);
        ASSERT_EQ(target_txn.get_data_length(), len);
        ASSERT_EQ(target_txn.get_command(), cmd);
    }

    void do_excl_txn_and_check(int id, bool is_load, uint64_t addr, size_t len,
                               bool expect_inval_or_success)
    {
        using namespace tlm;

        TlmGenericPayload txn;
        TlmResponseStatus ret;

        ExclusiveAccessTlmExtension ext;
        txn.set_extension(&ext);

// We could add the path ID here, but we will test the router's ability to do that.
// So the ID will effectively be given by the initiator and it's socket being used.
        if (is_load) {
            ret = m_initiators[id].do_read_with_txn_and_ptr(txn, addr, nullptr, len);
        } else {
            ret = m_initiators[id].do_write_with_txn_and_ptr(txn, addr, nullptr, len);
        }

        check_txn(is_load, addr, len, ret);

        if (is_load) {
            /* On an exclusive load, we check DMI invalidation coherence */
            ASSERT_EQ(last_dmi_inval_is_valid(), expect_inval_or_success);

            if (expect_inval_or_success) {
                ASSERT_EQ(m_last_dmi_inval_start, addr);
                ASSERT_EQ(m_last_dmi_inval_end, addr + len - 1);
                m_last_dmi_inval_valid = false;
            }
        } else {
            /*
             * On an exclusive store, we check the status of the store against
             * what we expect.
             */
            ExclusiveAccessTlmExtension::ExclusiveStoreStatus expected_sta;

            expected_sta = expect_inval_or_success
                ? ExclusiveAccessTlmExtension::EXCLUSIVE_STORE_SUCCESS
                : ExclusiveAccessTlmExtension::EXCLUSIVE_STORE_FAILURE;

            ASSERT_EQ(ext.get_exclusive_store_status(), expected_sta);
        }

        txn.clear_extension(&ext);
    }

    void do_txn_and_check(bool is_load, uint64_t addr, size_t len)
    {
        TlmResponseStatus ret;

        if (is_load) {
            ret = m_initiator.do_read_with_ptr(addr, nullptr, len);
        } else {
            ret = m_initiator.do_write_with_ptr(addr, nullptr, len);
        }

        check_txn(is_load, addr, len, ret);

        /* No DMI invalidation should have occured */
        ASSERT_FALSE(last_dmi_inval_is_valid());
    }

protected:
    void do_load_and_check(uint64_t addr, size_t len)
    {
        do_txn_and_check(true, addr, len);
    }

    void do_store_and_check(uint64_t addr, size_t len)
    {
        do_txn_and_check(false, addr, len);
    }

    void do_excl_load_and_check(int id, uint64_t addr, size_t len,
                                bool expect_dmi_inval)
    {
        do_excl_txn_and_check(id, true, addr, len, expect_dmi_inval);
    }

    void do_excl_store_and_check(int id, uint64_t addr, size_t len,
                                 bool expect_store_success)
    {
        do_excl_txn_and_check(id, false, addr, len, expect_store_success);
    }

    void do_good_dmi_request_and_check(uint64_t addr,
                                       uint64_t exp_start, uint64_t exp_end)
    {
        using namespace tlm;

        bool ret = m_initiator.do_dmi_request(addr);
        const tlm_dmi &dmi_data = m_initiator.get_last_dmi_data();

        ASSERT_TRUE(ret);
        ASSERT_EQ(exp_start, dmi_data.get_start_address());
        ASSERT_EQ(exp_end, dmi_data.get_end_address());
        ASSERT_EQ(uintptr_t(exp_start), uintptr_t(dmi_data.get_dmi_ptr()));
    }

    void do_bad_dmi_request_and_check(uint64_t addr)
    {
        using namespace tlm;

        bool ret = m_initiator.do_dmi_request(addr);

        ASSERT_FALSE(ret);
    }

    bool last_dmi_inval_is_valid() const
    {
        return m_last_dmi_inval_valid;
    }

    bool get_last_dmi_hint() const
    {
        return m_initiator.get_last_dmi_hint();
    }

    bool get_last_dmi_hint(int id) const
    {
        return m_initiators[id].get_last_dmi_hint();
    }

public:
    ExclusiveMonitorTestBench(const sc_core::sc_module_name &n)
        : TestBench(n)
        , m_monitor("exclusive-monitor")
        , m_initiators("initiator-testers",10)
        , m_initiator("initiator_tester")
        , m_target("target-tester", TARGET_MMIO_SIZE)
        , m_router("router")
    {
        using namespace std::placeholders;

        for (auto &i : m_initiators) {
            i.register_invalidate_direct_mem_ptr(
                std::bind(&ExclusiveMonitorTestBench::invalidate_direct_mem_ptr, this, _1, _2));
            i.socket.bind(m_router.target_socket);
        }
        m_initiator.register_invalidate_direct_mem_ptr(
            std::bind(&ExclusiveMonitorTestBench::invalidate_direct_mem_ptr, this, _1, _2));
        m_target.register_read_cb(
            std::bind(&ExclusiveMonitorTestBench::target_access, this, _1, _2, _3));
        m_target.register_write_cb(
            std::bind(&ExclusiveMonitorTestBench::target_access, this, _1, _2, _3));
        m_target.register_get_direct_mem_ptr_cb(
            std::bind(&ExclusiveMonitorTestBench::get_direct_mem_ptr, this, _1, _2));

        m_initiator.socket.bind(m_router.target_socket);
        m_router.add_target(m_monitor.front_socket, 0, TARGET_MMIO_SIZE+1);
        m_monitor.back_socket.bind(m_target.socket);
    }

    virtual ~ExclusiveMonitorTestBench()
    {}
};

#endif
