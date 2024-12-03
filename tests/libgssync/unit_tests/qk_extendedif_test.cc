/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <gtest/gtest.h>

#include "qk_extendedif.h"

class test_base : public sc_core::sc_module
{
public:
    test_base(const sc_core::sc_module_name& nm): sc_core::sc_module(nm)
    {
        SC_HAS_PROCESS(test_base);
        SC_THREAD(run);
    }

    ~test_base() { finalize_test(); }

    void run()
    {
        sc_core::wait(sc_core::SC_ZERO_TIME);
        run_test();
        sc_core::sc_stop();
    }

    void finalize_test() { ASSERT_EQ(sc_core::sc_get_status(), sc_core::SC_STOPPED) << "simulation incomplete"; }

    void run_test()
    {
        gs::tlm_quantumkeeper_extended qk;
        EXPECT_EQ(gs::SyncPolicy::SYSTEMC_THREAD, qk.get_thread_type());
        qk.reset();
        sc_core::sc_time quantum(1, sc_core::SC_MS);
        sc_core::sc_time local(100, sc_core::SC_NS);
        qk.set_global_quantum(quantum);
        EXPECT_EQ(quantum, qk.get_global_quantum());
        qk.start();
        EXPECT_EQ(sc_core::SC_ZERO_TIME, qk.time_to_sync());
        qk.inc(local);
        EXPECT_TRUE(qk.need_sync());
        qk.sync();
        EXPECT_FALSE(qk.need_sync());
        EXPECT_EQ(quantum - local, qk.time_to_sync());
        sc_core::sc_time wait(200, sc_core::SC_NS);
        sc_core::wait(wait);
        EXPECT_EQ(quantum - local - wait, qk.time_to_sync());
        qk.inc(quantum - local - wait);
        EXPECT_EQ(sc_core::SC_ZERO_TIME, qk.time_to_sync());
    }
};

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

int sc_main(int argc, char** argv)
{
    EXPECT_TRUE(false) << "sc_main called";
    return EXIT_FAILURE;
}

TEST(tlm_quantumkeeper_extended, basic_test)
{
    test_base tb("test_base");
    sc_core::sc_start(1, sc_core::SC_SEC);
}
