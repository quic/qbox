/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GENERIC_LUA_MODEL_TEST_H_
#define _GENERIC_LUA_MODEL_TEST_H_

#include <systemc>
#include <tlm>
#include <cciutils.h>
#include <argparser.h>
#include <tests/initiator-tester.h>
#include <tests/test-bench.h>
#include <ports/initiator-signal-socket.h>
#include <ports/target-signal-socket.h>
#include <generic_lua_model.h>
#include <gs_memory.h>
#include <router.h>
#include <reg_router.h>
#include <cstdint>
#include <algorithm>

class Test_generic_lua_model : public TestBench
{
public:
    SCP_LOGGER();
    Test_generic_lua_model(const sc_core::sc_module_name& n);

public:
    void test_reactions();

public:
    gs::gs_memory<> m_memory;
    gs::gs_memory<> m_reg_memory;
    gs::reg_router<> m_reg_router;
    gs::router<> m_router;
    InitiatorTester m_initiator;
    InitiatorSignalSocket<bool> m_signal_initiator0;
    InitiatorSignalSocket<bool> m_signal_initiator1;
    InitiatorSignalSocket<bool> m_reset_signal_initiator;
    TargetSignalSocket<bool> m_target_signal0;
    TargetSignalSocket<bool> m_target_signal1;
    std::unique_ptr<gs::generic_lua_model> m_gen_lua_model;
    uint32_t reset_counter;
};
#endif