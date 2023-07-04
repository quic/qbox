/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
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
        : m_pass("remote_pass")
        , m_router("remote_router")
        , m_loopback("loopback", true)
        , m_mem2("mem2")
        , m_mem3("mem3")
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
        gs::ConfigurableBroker m_broker(
        argc, argv,
        {
            { "remote.remote_pass.tlm_initiator_ports_num", cci::cci_value(2) },
            { "remote.remote_pass.tlm_target_ports_num", cci::cci_value(1) },
            /*number of signals (initiators & targets) used = 0, they are initialized to 0 by default*/
        });
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