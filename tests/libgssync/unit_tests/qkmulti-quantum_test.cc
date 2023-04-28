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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "greensocs/libgssync/qkmulti-quantum.h"

using testing::AnyOf;
using testing::Eq;

gs::tlm_quantumkeeper_extended* qk = nullptr;

bool done;
void unfinished_quantum() {
    // budget should be quantum
    sc_core::sc_time quantum(1, sc_core::SC_MS);
    sc_core::sc_time budget = qk->time_to_sync();
    EXPECT_EQ(budget, quantum);
    // increase local time offset by 100 SC_NS
    sc_core::sc_time inc(100, sc_core::SC_NS);
    qk->inc(inc);
    budget = qk->time_to_sync();
    EXPECT_EQ(budget, quantum - inc);
    EXPECT_FALSE(qk->need_sync());
    qk->sync();
    // Get budget, can be quantum or (quantum - inc), depending on how far SystemC has caught up
    budget = qk->time_to_sync();
    EXPECT_THAT(budget, AnyOf(Eq(quantum), Eq(quantum - inc)));
    qk->inc(inc);
    budget = qk->time_to_sync();
    // Same as above
    EXPECT_THAT(budget, AnyOf(Eq(quantum - inc), Eq(quantum - 2 * inc)));
    done = true;
    qk->stop();
}

void finished_quantum() {
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

int sc_main(int argc, char** argv) {
    qk = new gs::tlm_quantumkeeper_multi_quantum;
    sc_core::sc_time quantum(1, sc_core::SC_MS);
    tlm_utils::tlm_quantumkeeper::set_global_quantum(quantum);
    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}

TEST(qkmulti_quantum, unfinished_quantum) {
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

TEST(qkmulti_quantum, finished_quantum) {
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
