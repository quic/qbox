/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file uart-ibex-biflow-stdio-test.cc
 * @brief this is a test for stdio backend with ibex uart
 * the idea of the test is to redirect the stdin and stdout into pipes to test the stdio backend
 */

#include "greensocs/systemc-uarts/uart-ibex.h"

#include "greensocs/systemc-uarts/backends/char-backend.h"
#include <greensocs/systemc-uarts/backends/char/stdio.h>

#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/target-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <greensocs/gsutils/ports/target-signal-socket.h>
#include <systemc.h>

#define MAX_LEN 4096
char stdout_buffer[MAX_LEN + 1] = { 0 };
sc_event ev;
int r = 0;
class TestUart : public TestBench
{
    IbexUart m_uart_1;
    IbexUart m_uart_2;
    TargetSignalSocket<bool> m_irq_trigger;

public:
    CharBackendStdio u_backend_stdio_1, u_backend_stdio_2;
    InitiatorTester m_initiator_1, m_initiator_2;

    TestUart(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_uart_1("uart_1")
        , m_uart_2("uart_2")
        , u_backend_stdio_1("backend_1")
        , u_backend_stdio_2("backend_2")
        , m_initiator_1("initiator_1")
        , m_irq_trigger("irq_trigger")
        , m_initiator_2("initiator_2")
    {
        m_initiator_1.socket.bind(m_uart_1.socket);
        m_initiator_2.socket.bind(m_uart_2.socket);
        m_uart_1.backend_socket.bind(u_backend_stdio_1.socket);
        m_uart_2.backend_socket.bind(u_backend_stdio_2.socket);

        m_uart_2.irq_rxwatermark.bind(m_irq_trigger);
        auto cb = std::bind(&TestUart::fn_cb, this, std::placeholders::_1);
        m_irq_trigger.register_value_changed_cb(cb);
    }
    void fn_cb(const bool& val)
    {
        std::cout << "the val from irq is: " << val << std::endl;
        if (val) {
            ev.notify();
            r++;
        }
    }
};

TEST_BENCH(TestUart, IbexStdioBackend)
{
    std::string str = "Qualcomm";
    //___________ redirect stdout ____________//

    int out_pipe[2];
    int saved_stdout;

    saved_stdout = dup(STDOUT_FILENO); // save stdout for display later

    if (pipe(out_pipe) != 0) { // make a pipe
        exit(1);
    }

    dup2(out_pipe[1], STDOUT_FILENO); // redirect stdout to the pipe
    close(out_pipe[1]);

    for (std::string::iterator itr = str.begin(); itr != str.end(); ++itr) {
        m_initiator_1.do_write(IbexUart::REG_WDATA, *itr); // data
        sc_core::wait(1, sc_core::SC_NS);                  // wait for txn transication
    }

    fflush(stdout);
    read(out_pipe[0], stdout_buffer, MAX_LEN); // read from pipe into stdout_buffer
    dup2(saved_stdout, STDOUT_FILENO);         // reconnect stdout for testing
    fflush(stdout);

    std::cout << "the redirected string from stdout is: " << stdout_buffer << std::endl;

    //___________ redirect stdin ____________//
    char stdout_buffer[9] = "Qualcomm";
    m_initiator_2.do_write(IbexUart::REG_CTRL, IbexUart::FIELD_CTRL_RXENABLE);
    m_initiator_2.do_write(IbexUart::REG_ITEN, IbexUart::FIELD_IT_RXWATERMARK);
    sc_core::wait(1, sc_core::SC_NS);
    for (std::string::size_type i = 0; i < std::strlen(stdout_buffer); i++) {
        //  We should write the string to stdin, e.g.
        //        ioctl(0, TIOCSTI, &stdout_buffer[i]);
        //          but since it's hard to control STDIN in the test harness
        //          bypass and enqueue directly.
        u_backend_stdio_2.enqueue(stdout_buffer[i]);
        std::cout << "the sent character is: " << stdout_buffer[i] << std::endl;
    }
    if (!r) wait(ev);

    char data;
    for (std::string::size_type i = 0; i < std::strlen(stdout_buffer); i++) {
        sc_core::wait(1, sc_core::SC_NS);
        ASSERT_EQ(m_initiator_2.do_read(IbexUart::REG_RDATA, data), tlm::TLM_OK_RESPONSE);
        std::cout << "the recived character is: " << data << std::endl;
    }

    sc_core::wait(1, sc_core::SC_NS);
    sc_core::sc_stop();
}
int sc_main(int argc, char* argv[])
{
    gs::ConfigurableBroker m_broker(argc, argv, { { "IbexStdioBackend.backend_1.read_write", cci::cci_value(false) } });

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
