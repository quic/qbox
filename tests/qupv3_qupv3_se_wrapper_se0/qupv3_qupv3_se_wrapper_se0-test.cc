/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

#include <greensocs/base-components/router.h>
#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/reg_model_maker/reg_model_maker.h>

#include "quic/qupv3_qupv3_se_wrapper_se0/qupv3_qupv3_se_wrapper_se0.h"

#include "greensocs/systemc-uarts/backends/char-backend.h"
#include <greensocs/systemc-uarts/backends/char/stdio.h>

#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/target-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>
#include <greensocs/gsutils/ports/target-signal-socket.h>

class TestUart : public TestBench
{
public:
    qupv3_qupv3_se_wrapper_se0 m_uart;
    CharBackendStdio m_backend_stdio;
    gs::Router<> m_router;
    InitiatorTester m_initiator;
    TargetSignalSocket<bool> m_signal;
    TestUart(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_uart("uart")
        , m_backend_stdio("backend")
        , m_router("router")
        , m_initiator("initiator")
        , m_signal("irq")
    {
        m_initiator.socket.bind(m_router.target_socket);
        m_router.initiator_socket(m_uart.target_socket);
        m_uart.backend_socket.bind(m_backend_stdio.socket);

        m_uart.irq.bind(m_signal);
    }
};

/* Test QUP UART Tx */
TEST_BENCH(TestUart, UartWrite)
{
    m_initiator.do_write(0x600, 0x08000000);
    m_initiator.do_write(0x270, 0x5); // print ABCDE but not X

    m_initiator.do_write(0x700, (uint32_t)'DCBA');
    wait(10, sc_core::SC_NS);
    m_initiator.do_write(0x700, (uint32_t)'E');
    wait(10, sc_core::SC_NS);
    m_initiator.do_write(0x700, (uint32_t)'X');
    wait(10, sc_core::SC_NS);
    m_initiator.do_write(0x618, 0xffffffff);

    sc_core::wait(1, sc_core::SC_NS);
    sc_core::sc_stop();
}

int sc_main(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(50));      // make the msg type column a bit tighter
    gs::ConfigurableBroker m_broker(argc, argv, { { "log_level", cci::cci_value(5) } });

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
