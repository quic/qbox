/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "router.h"
#include "pass.h"
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/target-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>
#include <systemc>
#include <tlm>
#include <scp/report.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <vector>

static constexpr size_t NB_TARGETS = 4;

class RouterTestBenchSimple : public TestBench
{
public:
    std::vector<uint64_t> address = { 0, 257, 514, 700 };
    std::vector<size_t> size = { 256, 256, 256, 256 };
    std::vector<uint64_t> target_size;

    using TlmResponseStatus = InitiatorTester::TlmResponseStatus;
    using TlmGenericPayload = InitiatorTester::TlmGenericPayload;
    using TlmDmi = InitiatorTester::TlmDmi;

private:
    InitiatorTester m_initiator;
    gs::router<> m_router;
    std::vector<TargetTester*> m_target;
    gs::pass<> m_pass;

    bool m_overlap_address = false;
    bool m_overlap_size = false;

    uint8_t emptydata[1000] = { 0xDE, 0xAD, 0xBE, 0xEF };

    /* Initiator callback */
    void invalidate_direct_mem_ptr(uint64_t start_range, uint64_t end_range)
    {
        ADD_FAILURE(); /* we don't expect any invalidation */
    }

    /* Target callbacks */
    TlmResponseStatus target_access(int id, uint64_t addr, uint8_t* data, size_t len)
    {
        m_target[id]->get_cur_txn().set_dmi_allowed(true);
        return tlm::TLM_OK_RESPONSE;
    }

    void overlap(int id)
    {
        // We test if the address we want to test has an overlap with another target
        for (int i = 0; i < NB_TARGETS; i++) {
            // Loop to avoid testing the target on itself
            if (id != i) {
                m_overlap_address = address[id] >= address[i] && address[id] < address[i] + size[i];
                m_overlap_size = address[id] + size[id] > address[i] && address[id] + size[id] < address[i] + size[i];
            }
        }
    }

    void check_txn(int id, bool is_load, uint64_t addr, size_t len, TlmResponseStatus ret)
    {
        using namespace tlm;

        tlm_command cmd = is_load ? TLM_READ_COMMAND : TLM_WRITE_COMMAND;

        // Check if there is an Out of bound
        if (addr >= target_size[id]) {
            if (ret == TLM_GENERIC_ERROR_RESPONSE) {
                ASSERT_FALSE(m_target[id]->last_txn_is_valid());
                return;
            } else if (ret == TLM_ADDRESS_ERROR_RESPONSE) {
                ASSERT_FALSE(m_target[id]->last_txn_is_valid());
                return;
            }

            // Check if there is an Overlap
        } else {
            if (ret == TLM_GENERIC_ERROR_RESPONSE) {
                ASSERT_FALSE(m_target[id]->last_txn_is_valid());
                return;
            } else if (ret == TLM_ADDRESS_ERROR_RESPONSE) {
                ASSERT_TRUE(m_target[id]->last_txn_is_valid());
                return;
            }
            ASSERT_TRUE(m_target[id]->last_txn_is_valid());
        }
        const TlmGenericPayload& target_txn = m_target[id]->get_last_txn();

        ASSERT_EQ((target_txn.get_address() + address[id]), addr);
        ASSERT_EQ(target_txn.get_data_length(), len);
        ASSERT_EQ(target_txn.get_command(), cmd);
    }

    void check_txn_debug(int id, bool is_load, uint64_t addr, size_t len, int ret)
    {
        using namespace tlm;

        ret = m_initiator.get_last_transport_debug_ret();

        tlm_command cmd = is_load ? TLM_READ_COMMAND : TLM_WRITE_COMMAND;

        // Check if there is an Out of bound
        if (addr >= target_size[id]) {
            if (ret == TLM_GENERIC_ERROR_RESPONSE) {
                ASSERT_FALSE(m_target[id]->last_txn_is_valid());
                return;
            } else if (ret == 0) {
                ASSERT_FALSE(m_target[id]->last_txn_is_valid());
                return;
            }

            // Check if there is an Overlap
        } else {
            if (ret == TLM_GENERIC_ERROR_RESPONSE) {
                ASSERT_FALSE(m_target[id]->last_txn_is_valid());
                return;
            } else if (ret == 0) {
                ASSERT_TRUE(m_target[id]->last_txn_is_valid());
                return;
            }
            ASSERT_TRUE(m_target[id]->last_txn_is_valid());
        }

        const TlmGenericPayload& target_txn = m_target[id]->get_last_txn();

        ASSERT_EQ((target_txn.get_address() + address[id]), addr);
        ASSERT_EQ(target_txn.get_data_length(), len);
        ASSERT_EQ(target_txn.get_command(), cmd);
    }

    void do_txn_and_check(int id, bool is_load, uint64_t addr, size_t len, bool debug)
    {
        TlmResponseStatus ret;
        overlap(id);

        if (m_overlap_address || m_overlap_size) {
            SCP_INFO(SCMOD) << "An overlap of address or size was detected during the test" << std::endl;
        } else {
            if (is_load) {
                ret = m_initiator.do_read_with_ptr(addr, emptydata, len, debug);
            } else {
                ret = m_initiator.do_write_with_ptr(addr, emptydata, len, debug);
            }
            check_txn(id, is_load, addr, len, ret);
        }
    }

    void do_txn_debug_and_check(int id, bool is_load, uint64_t addr, size_t len, bool debug)
    {
        int ret;
        overlap(id);

        if (m_overlap_address || m_overlap_size) {
            SCP_INFO(SCMOD) << "An overlap of address or size was detected during the test" << std::endl;
        } else {
            if (is_load) {
                ret = m_initiator.do_read_with_ptr(addr, emptydata, len, debug);
            } else {
                ret = m_initiator.do_write_with_ptr(addr, emptydata, len, debug);
            }

            check_txn_debug(id, is_load, addr, len, ret);
        }
    }

    bool get_direct_mem_ptr(int id, uint64_t addr, TlmDmi& dmi_data)
    {
        if (addr >= target_size[id]) {
            return false;
        } else {
            dmi_data.allow_read_write();
            dmi_data.set_dmi_ptr(nullptr);
            dmi_data.set_start_address(0);
            dmi_data.set_end_address(size[id] - 1);

            return true;
        }
    }

protected:
    void do_load_and_check(int id, uint64_t addr, size_t len, bool debug = false)
    {
        if (debug == false) {
            do_txn_and_check(id, true, addr, len, debug);
        } else {
            do_txn_debug_and_check(id, true, addr, len, debug);
        }
    }

    void do_store_and_check(int id, uint64_t addr, size_t len, bool debug = false)
    {
        if (debug == false) {
            do_txn_and_check(id, false, addr, len, debug);
        } else {
            do_txn_debug_and_check(id, false, addr, len, debug);
        }
    }

    void do_good_dmi_request_and_check(int id, uint64_t addr, uint64_t exp_start, uint64_t exp_end)
    {
        overlap(id);

        if (m_overlap_address || m_overlap_size) {
            SCP_INFO(SCMOD) << "An overlap of address or size was detected during the test" << std::endl;
        } else {
            using namespace tlm;

            bool ret = m_initiator.do_dmi_request(addr);
            const tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();

            ASSERT_TRUE(ret);
            ASSERT_EQ(exp_start, dmi_data.get_start_address());
            ASSERT_EQ(exp_end, dmi_data.get_end_address());
        }
    }

    void do_bad_dmi_request_and_check(int id, uint64_t addr)
    {
        overlap(id);

        if (m_overlap_address || m_overlap_size) {
            SCP_INFO(SCMOD) << "An overlap of address or size was detected during the test" << std::endl;
        } else {
            using namespace tlm;

            bool ret = m_initiator.do_dmi_request(addr);

            ASSERT_FALSE(ret);
        }
    }

public:
    RouterTestBenchSimple(const sc_core::sc_module_name& n)
        : TestBench(n), m_initiator("initiator"), m_router("router"), m_pass("pass"), m_target()
    {
        int id = 0;

        for (int i = 0; i < NB_TARGETS; i++) {
            char txt[20];
            snprintf(txt, 20, "Target_%d", i);
            m_target.push_back(new TargetTester(txt, size[i]));
            target_size.push_back(address[i] + size[i]);
        }

        m_initiator.register_invalidate_direct_mem_ptr(
            [this](uint64_t start, uint64_t end) { invalidate_direct_mem_ptr(start, end); });
        for (auto& t : m_target) {
            t->register_write_cb([id, this](uint64_t addr, uint8_t* data, size_t size) -> TlmResponseStatus {
                return target_access(id, addr, data, size);
            });

            t->register_read_cb([id, this](uint64_t addr, uint8_t* data, size_t size) -> TlmResponseStatus {
                return target_access(id, addr, data, size);
            });

            t->register_debug_read_cb([id, this](uint64_t addr, uint8_t* data, size_t size) -> TlmResponseStatus {
                return target_access(id, addr, data, size);
            });

            t->register_debug_write_cb([id, this](uint64_t addr, uint8_t* data, size_t size) -> TlmResponseStatus {
                return target_access(id, addr, data, size);
            });

            t->register_get_direct_mem_ptr_cb(
                [id, this](uint64_t addr, TlmDmi& dmi_data) -> bool { return get_direct_mem_ptr(id, addr, dmi_data); });
            id++;
        }

        m_initiator.socket.bind(m_pass.target_socket);
        m_pass.initiator_socket(m_router.target_socket);
        for (int i = 0; i < NB_TARGETS; i++) {
            m_router.add_target(m_target[i]->socket, address[i], size[i]);
        }
    }

    virtual ~RouterTestBenchSimple()
    {
        while (!m_target.empty()) {
            delete m_target.back();
            m_target.pop_back();
        }
    }
};