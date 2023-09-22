/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <tlm>
#include <cci_configuration>
#include <scp/report.h>

#include <greensocs/libgsutils.h>

#include "memory.h"
#include "router.h"
#include "remote.h"
#include "pass.h"

#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

std::string getexepath()
{
    char result[1024] = { 0 };
#ifdef _WIN32
    GetModuleFileNameW(NULL, result, 1024);
    return result;
#elif __APPLE__
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) != 0) {
        return NULL;
    }
    return result;
#else
    ssize_t count = readlink("/proc/self/exe", result, 1024);
    return std::string(result, (count > 0) ? count : 0);
#endif
}

class RemotePassTest : public TestBench
{
private:
    gs::PassRPC<> m_pass; // should be first model, to handle BEOE
    InitiatorTester m_initiator;
    InitiatorTester m_initiator_dma;
    gs::Router<> m_router;
    gs::Memory<> m_mem1;
    gs::pass<> m_log;

public:
    void do_write_read_check(uint64_t addr)
    {
        uint64_t v1 = 0xdeadbeef, v2;
        ASSERT_EQ(m_initiator.do_write(addr, v1), tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(m_initiator.do_read(addr, v2), tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(v1, v2);
    }
    void do_write_read_check_larger(uint64_t addr)
    {
        uint64_t a1[3] = { 0xdeadbeeffeedbeef, 0xbeeffeedb00bb00b, 0xdeadf00dd000bad0 };
        uint64_t a2[3] = { 0x0, 0x0, 0x0 };

        ASSERT_EQ(m_initiator.do_write_with_ptr(addr, (uint8_t*)&a1, sizeof(uint64_t) * 3, false),
                  tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(m_initiator.do_read_with_ptr(addr, (uint8_t*)&a2, sizeof(uint64_t) * 3, false), tlm::TLM_OK_RESPONSE);
        for (int i = 0; i < 3; i++) {
            ASSERT_EQ(a1[i], a2[i]);
        }
    }
    void do_remote_write_read_check(uint64_t addr)
    {
        uint64_t v1 = 0xfad0f00d, v2;
        ASSERT_EQ(m_initiator_dma.do_write(addr, v1), tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(m_initiator_dma.do_read(addr, v2), tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(v1, v2);
    }
    void do_dmi_write_read_check(uint64_t addr)
    {
        ASSERT_TRUE(m_initiator.do_dmi_request(addr));
        tlm::tlm_dmi dmi = m_initiator.get_last_dmi_data();
        uint64_t* d = (uint64_t*)dmi.get_dmi_ptr();
        uint64_t v1 = 0x900df00d, v2;
        d[0x0] = v1;
        v2 = d[0x0];
        ASSERT_EQ(v1, v2);
    }

public:
    RemotePassTest(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_initiator("initiator")
        , m_initiator_dma("initiator_dma")
        , m_router("router")
        , m_pass("pass")
        , m_mem1("mem1")
        , m_log("local")
    {
        SCP_INFO(SCMOD) << " path:  = " << getexepath();

        // main initiator to access local/remote memory
        m_initiator.socket.bind(m_router.target_socket);

        // NB the order must match the order of sockets bound to the initiator port of the other end
        // bridge to remote memories.
        m_router.initiator_socket.bind(m_pass.target_sockets[0]);
        // send DMA instructions to the remote, to access back to our memory
        m_initiator_dma.socket.bind(m_pass.target_sockets[1]);

        // local memory
        m_router.initiator_socket.bind(m_log.target_socket);
        m_log.initiator_socket.bind(m_mem1.socket);
        // DMA connection back from the remote.
        m_pass.initiator_sockets[0].bind(m_router.target_socket);
    }
    virtual ~RemotePassTest() {}
};
