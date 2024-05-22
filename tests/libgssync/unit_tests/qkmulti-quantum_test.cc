/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "qkmulti-quantum.h"

using testing::AnyOf;
using testing::Eq;

gs::tlm_quantumkeeper_extended* qk = nullptr;

bool done;
void unfinished_quantum()
{
    // budget should be quantum
    sc_core::sc_time quantum(1, sc_core::SC_MS);
    sc_core::sc_time budget = qk->time_to_sync();
    EXPECT_EQ(budget, quantum);
    // increase local time offset by 100 SC_NS
    sc_core::sc_time inc(100, sc_core::SC_NS);
    qk->inc(inc);
    budget = qk->time_to_sync();
    EXPECT_THAT(budget, AnyOf(Eq(quantum), Eq(quantum - inc)));
    EXPECT_FALSE(qk->need_sync());
    qk->sync();
    // Get budget, can be quantum or (quantum - inc), depending on how far SystemC has caught up
    budget = qk->time_to_sync();
    EXPECT_THAT(budget, AnyOf(Eq(quantum), Eq(quantum - inc)));
    qk->inc(inc);
    budget = qk->time_to_sync();
    // Same as above
    EXPECT_THAT(budget, AnyOf(Eq(quantum), Eq(quantum - inc), Eq(quantum - 2 * inc)));
    done = true;
    qk->stop();
}

void finished_quantum()
{
    // budget should be quantum
    sc_core::sc_time quantum(1, sc_core::SC_MS);
    sc_core::sc_time budget = qk->time_to_sync();
    EXPECT_EQ(budget, quantum);
    sc_core::sc_time inc(100, sc_core::SC_NS);
    qk->inc(inc);
    budget = qk->time_to_sync();
    EXPECT_EQ(budget, quantum - inc);
    EXPECT_FALSE(qk->need_sync());
    qk->sync();
    budget = qk->time_to_sync();
    EXPECT_THAT(budget, AnyOf(Eq(quantum), Eq(quantum - inc)));
    qk->inc(budget);
    qk->sync();
    budget = qk->time_to_sync();
    EXPECT_LE(budget, quantum);
    done = true;
    qk->stop();
}

int sc_main(int argc, char** argv)
{
    qk = new gs::tlm_quantumkeeper_multi_quantum;
    sc_core::sc_time quantum(1, sc_core::SC_MS);
    tlm_utils::tlm_quantumkeeper::set_global_quantum(quantum);
    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}

TEST(qkmulti_quantum, unfinished_quantum)
{
    done = false;
    qk->start();
    qk->reset();
    std::thread t1(unfinished_quantum);
    while (sc_core::sc_pending_activity() || !done) {
        if (sc_core::sc_pending_activity()) {
            sc_core::sc_time t = sc_core::sc_time_to_pending_activity();
            sc_start(t);
        }
    }
    t1.join();
    qk->stop();
}

TEST(qkmulti_quantum, finished_quantum)
{
    done = false;
    qk->start();
    qk->reset();
    std::thread t1(finished_quantum);
    while (sc_core::sc_pending_activity() || !done) {
        if (sc_core::sc_pending_activity()) {
            sc_core::sc_time t = sc_core::sc_time_to_pending_activity();
            sc_start(t);
        }
    }
    t1.join();
}
