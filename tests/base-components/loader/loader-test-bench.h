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

#include <systemc>
#include <cci_configuration>

#include <greensocs/gsutils/luafile_tool.h>
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
    gs::Router<> m_router;
    gs::Memory<> m_rom1;
    gs::Memory<> m_rom2;
    gs::Memory<> m_rom3;

    gs::Loader<> m_loader;

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
