/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file uart-biflow-backend-socket-test.cc
 * @brief this is a test for socket backend with Pl011 uart
 * the first uart is connected to the server socket
 * the second uart is connected to the client socket
 * both socket have the same address which is "127.0.0.1:8000"
 * the test start with writing characters to the first uart and expect to read them from the second
 * uart
 */

#include "greensocs/systemc-uarts/uart-pl011-bf.h"

#include "greensocs/systemc-uarts/backends/char-backend.h"
#include <greensocs/systemc-uarts/backends/char-bf/socket.h>
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/target-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>

#include <greensocs/gsutils/ports/target-signal-socket.h>

#include <systemc.h>

sc_event ev;
int r = 0;

class TestUart : public TestBench
{
    Uart m_uart_1, m_uart_2;
    CharBackendSocket server, client;
    TargetSignalSocket<bool> m_irq_trigger;

public:
    InitiatorTester m_initiator_1, m_initiator_2;
    TestUart(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_uart_1("uart_1")
        , m_uart_2("uart_2")
        , client("s_client")
        , server("s_server")
        , m_initiator_1("initiator_1")
        , m_irq_trigger("irq_trigger")
        , m_initiator_2("initiator_2")
    {
        m_uart_1.backend_socket.bind(server.socket);
        m_uart_2.backend_socket.bind(client.socket);

        m_initiator_1.socket.bind(m_uart_1.socket);
        m_initiator_2.socket.bind(m_uart_2.socket);

        m_uart_2.irq.bind(m_irq_trigger);

        auto cb = std::bind(&TestUart::fn_cb, this, std::placeholders::_1);
        m_irq_trigger.register_value_changed_cb(cb);
    }

    void fn_cb(const bool& val)
    {
        std::cout << "the val from irq_2 is: " << val << std::endl;
        if (val) {
            ev.notify();
            r++;
        }
    }
};

TEST_BENCH(TestUart, SocketBackend)
{
    m_initiator_1.do_write(0, 'H'); // data
    m_initiator_1.do_write(0, 'i'); // data

    m_initiator_2.do_write(14 << 2, 0x10); // #define PL011_INT_RX 0x10 - s->int_enabled
    m_initiator_2.do_write(11 << 2, 0x10); // pl011_set_read_trigger();
    if (!r) wait(ev);

    uint8_t data;
    ASSERT_EQ(m_initiator_2.do_read(0x00, data), tlm::TLM_OK_RESPONSE);

    std::cout << "the recived string is: " << data << std::endl;

    ASSERT_EQ(m_initiator_2.do_read(0x00, data), tlm::TLM_OK_RESPONSE);

    std::cout << "the recived string is: " << data << std::endl;
    sc_core::wait(1, sc_core::SC_NS);
    sc_core::sc_stop();
}

int sc_main(int argc, char* argv[])
{
    gs::ConfigurableBroker m_broker(argc, argv,
                                    {
                                        { "SocketBackend.s_client.address", cci::cci_value("127.0.0.1:8000") },
                                        { "SocketBackend.s_client.server", cci::cci_value(false) },
                                        { "SocketBackend.s_server.nowait", cci::cci_value(false) },
                                        { "SocketBackend.s_client.nowait", cci::cci_value(false) },
                                        { "SocketBackend.s_server.address", cci::cci_value("127.0.0.1:8000") },
                                    });

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
