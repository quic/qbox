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

#include "greensocs/systemc-uarts/uart-pl011.h"

#include "greensocs/systemc-uarts/backends/char-backend.h"
#include <greensocs/systemc-uarts/backends/char/stdio.h>
#if 0
#include <greensocs/systemc-uarts/backends/char/ncurses.h>
#include <greensocs/systemc-uarts/backends/char/socket.h>
#endif

#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/target-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>

class TestUart : public TestBench {
    Uart m_uart;
    CharBackendStdio u_backend_stdio;
public:
    InitiatorTester m_initiator;

    TestUart(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_uart("uart")
        , u_backend_stdio("backend")
        , m_initiator("initiator")
    {
        m_initiator.socket.bind(m_uart.socket);
        m_uart.set_backend(&u_backend_stdio);
    }
};

TEST_BENCH(TestUart, UartWrite)
{
    m_initiator.do_write(0x0,"h");
    m_initiator.do_write(0x0,"i");    

    sc_core::wait(1, sc_core::SC_NS);
    sc_core::sc_stop();
//    ASSERT_TRUE(m_mock_module.expected_result());
}

int sc_main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
