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

std::string getexepath() {
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
    void do_write_read_check(uint64_t addr) {
        uint64_t v1 = 0xdeadbeef, v2;
        ASSERT_EQ(m_initiator.do_write(addr, v1), tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(m_initiator.do_read(addr, v2), tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(v1, v2);
    }
    void do_remote_write_read_check(uint64_t addr) {
        uint64_t v1 = 0xfad0f00d, v2;
        ASSERT_EQ(m_initiator_dma.do_write(addr, v1), tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(m_initiator_dma.do_read(addr, v2), tlm::TLM_OK_RESPONSE);
        ASSERT_EQ(v1, v2);
    }
    void do_dmi_write_read_check(uint64_t addr) {
        ASSERT_TRUE(m_initiator.do_dmi_request(addr));
        tlm::tlm_dmi dmi=m_initiator.get_last_dmi_data();
        uint64_t *d=(uint64_t *)dmi.get_dmi_ptr();
        uint64_t v1 = 0x900df00d, v2;
        d[0x0]=v1;
        v2=d[0x0];
        ASSERT_EQ(v1,v2);
    }
public:
    RemotePassTest(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_initiator("initiator")
        , m_initiator_dma("initiator_dma")        
        , m_router("router")
        , m_pass("pass", getexepath() + "-remote")
        , m_mem1("mem1") 
        , m_log("local", true)
        {
        std::cout << " path:  = " << getexepath() << "\n";

        // main initiator to access local/remote memory
        m_initiator.socket.bind(m_router.target_socket);

        // NB the order must match the order of sockets bound to the initiator port of the other end
        // bridge to remote memories.
        m_router.initiator_socket.bind(m_pass.target_socket);
        // send DMA instructions to the remote, to access back to our memory
        m_initiator_dma.socket.bind(m_pass.target_socket);

        // local memory
        m_router.initiator_socket.bind(m_log.target_socket);
        m_log.initiator_socket.bind(m_mem1.socket);        
        // DMA connection back from the remote.
        m_pass.initiator_socket.bind(m_router.target_socket);

    }
    virtual ~RemotePassTest() {}
};
