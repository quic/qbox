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

class RemoteTest : public sc_core::sc_module
{
    gs::PassRPC<> m_pass;
    gs::pass<> m_loopback;

    gs::Router<> m_router;
    gs::Memory<> m_mem2;
    gs::Memory<> m_mem3;
    InitiatorTester* m_initiator;

public:
    SC_HAS_PROCESS(RemoteTest);
    RemoteTest(const sc_core::sc_module_name& n)
        : m_pass("remote_pass"), m_router("remote_router"), m_loopback("loopback", true), m_mem2("mem2"), m_mem3("mem3")
    {
        auto m_broker = cci::cci_get_broker();

        m_router.initiator_socket.bind(m_mem2.socket);
        m_router.initiator_socket.bind(m_mem3.socket);

        m_pass.initiator_sockets[0].bind(m_router.target_socket);
        m_pass.initiator_sockets[1].bind(m_loopback.target_socket);
        m_loopback.initiator_socket.bind(m_pass.target_sockets[0]);
    }

    virtual ~RemoteTest() {}

    void test_bench_body()
    {
        uint64_t v1 = 0xdeadbeef, v2;
        assert(m_initiator->do_write(0x0, v1) == tlm::TLM_OK_RESPONSE);
        for (int i = 0; i < 1000; i++) {
            assert(m_initiator->do_read(0x22020, v2) == tlm::TLM_OK_RESPONSE);
            sc_core::wait(1, sc_core::SC_MS);
        }
    }
};

int sc_main(int argc, char* argv[])
{
    try {
        SCP_INFO("RemoteMain") << "Remote started";
        gs::ConfigurableBroker m_broker(argc, argv);
        RemoteTest remote("remote");
        SCP_INFO("RemoteMain") << "END OF ELAB for remote";
        sc_core::sc_start();
    } catch (std::runtime_error const& e) {
        std::cerr << "Error: '" << e.what() << "'\n";
        exit(1);
    } catch (const std::exception& exc) {
        std::cerr << "Error: '" << exc.what() << "'\n";
        exit(1);
    } catch (...) {
        std::cerr << "Unknown error!\n";
        exit(2);
    }
    return 0;
}