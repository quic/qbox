/*
 * Copyright (c) 2022 GreenSocs
 *
 *  SPDX-License-Identifier: BSD-3-Clause
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

class TestUart : public TestBench
{
    Uart m_uart;
    char_backend_stdio u_backend_stdio;

public:
    InitiatorTester m_initiator;

    TestUart(const sc_core::sc_module_name& n)
        : TestBench(n), m_uart("uart"), u_backend_stdio("backend"), m_initiator("initiator")
    {
        m_initiator.socket.bind(m_uart.socket);
        m_uart.set_backend(&u_backend_stdio);
    }
};

TEST_BENCH(TestUart, UartWrite)
{
    m_initiator.do_write(0x0, 'h');
    m_initiator.do_write(0x0, 'i');

    sc_core::wait(1, sc_core::SC_NS);
    sc_core::sc_stop();
    //    ASSERT_TRUE(m_mock_module.expected_result());
}

int sc_main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
