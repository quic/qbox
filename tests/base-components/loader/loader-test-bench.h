/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <cci_configuration>

#include <greensocs/gsutils/argparser.h>
#include <greensocs/libgsutils.h>

#include "memory.h"
#include "router.h"
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class LoaderTest : public TestBench
{
protected:
    InitiatorTester m_initiator;
    gs::router<> m_router;
    gs::memory<> m_rom1;
    gs::memory<> m_rom2;
    gs::memory<> m_rom3;

    gs::loader<> m_loader;

    void do_bus_binding()
    {
        m_router.initiator_socket.bind(m_rom1.socket);
        m_router.initiator_socket.bind(m_rom2.socket);
        m_router.initiator_socket.bind(m_rom3.socket);
        m_router.add_initiator(m_initiator.socket);
        // General loader
        m_loader.initiator_socket.bind(m_router.target_socket);
    }

public:
    LoaderTest(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_initiator("initiator")
        , m_router("router")
        , m_rom1("rom1")
        , m_rom2("rom2")
        , m_rom3("rom3")
        , m_loader("load")
    {
        do_bus_binding();
    }
};
