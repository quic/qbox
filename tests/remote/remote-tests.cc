/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

#include "remote-bench.h"
#include <cci/utils/broker.h>

// Simple load and store into the Target 1 and 2
TEST_BENCH(RemotePassTest, SimpleReadWriteMem0)
{
    std::cout << "Test 1\n";
    for (int i = 0; i < 1; i++) {
        do_write_read_check(0x11000);
    }
    std::cout << "Test 2\n";
    for (int i = 0; i < 1; i++) {
        do_write_read_check(0x22000);
    }
    std::cout << "Test 3\n";
    for (int i = 0; i < 1; i++) {
        do_remote_write_read_check(0x11000);
    }
    std::cout << "Test 4\n";
    for (int i = 0; i < 1; i++) {
        do_dmi_write_read_check(0x23000);
    }
    std::cout << "Looks OK\n";
    sc_core::sc_stop();
}

int sc_main(int argc, char* argv[])
{
    gs::ConfigurableBroker m_broker(
        argc, argv,
        {
            { "test_bench.remote_router.target_socket.address", cci::cci_value(0x20000) },
            { "test_bench.remote_router.target_socket.size", cci::cci_value(0x10000) },
            { "test_bench.remote_router.target_socket.relative_addresses", cci::cci_value(false) },

            { "test_bench.mem1.target_socket.address", cci::cci_value(0x11000) },
            { "test_bench.mem1.target_socket.size", cci::cci_value(0x100) },
            { "test_bench.mem2.target_socket.address", cci::cci_value(0x22000) },
            { "test_bench.mem2.target_socket.size", cci::cci_value(0x100) },
            { "test_bench.mem3.target_socket.address", cci::cci_value(0x23000) },
            { "test_bench.mem3.target_socket.size", cci::cci_value(0x100) },

            { "test_bench.mem1.verbose", cci::cci_value(true) },
            { "test_bench.mem2.verbose", cci::cci_value(true) },
            { "test_bench.mem3.verbose", cci::cci_value(true) },

            { "test_bench.mem1.shared_memory", cci::cci_value(true) },
            { "test_bench.mem2.shared_memory", cci::cci_value(true) },
            { "test_bench.mem3.shared_memory", cci::cci_value(true) },
        });

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}