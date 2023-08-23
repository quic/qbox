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
#include <scp/report.h>

// Simple load and store into the Target 1 and 2
TEST_BENCH(RemotePassTest, test_bench)
{
    SCP_INFO(SCMOD) << "Test 1";
    for (int i = 0; i < 10; i++) {
        do_write_read_check(0x11000);
    }
    SCP_INFO(SCMOD) << "Test 2";
    for (int i = 0; i < 10; i++) {
        do_write_read_check(0x22000);
    }
    SCP_INFO(SCMOD) << "Test 2l";
    for (int i = 0; i < 10; i++) {
        do_write_read_check_larger(0x22000 + (i * 8));
    }
    SCP_INFO(SCMOD) << "Test 3";
    for (int i = 0; i < 10; i++) {
        // do_remote_write_read_check(0x11000);
    }
    SCP_INFO(SCMOD) << "Test 4";
    for (int i = 0; i < 10; i++) {
        do_dmi_write_read_check(0x23000);
    }
    SCP_INFO(SCMOD) << "Looks OK";
    sc_core::sc_stop();
}

int sc_main(int argc, char* argv[])
{
    gs::ConfigurableBroker m_broker(argc, argv,
                                    {
                                        { "test_bench.mem1.target_socket.address", cci::cci_value(0x11000) },
                                        { "test_bench.mem1.target_socket.size", cci::cci_value(0x1000) },
                                        { "test_bench.pass.mem2.target_socket.address", cci::cci_value(0x22000) },
                                        { "test_bench.pass.mem2.target_socket.size", cci::cci_value(0x1000) },
                                        { "test_bench.pass.mem3.target_socket.address", cci::cci_value(0x23000) },
                                        { "test_bench.pass.mem3.target_socket.size", cci::cci_value(0x1000) },

                                        { "test_bench.mem1.verbose", cci::cci_value(true) },
                                        { "test_bench.pass.mem2.verbose", cci::cci_value(true) },
                                        { "test_bench.pass.mem3.verbose", cci::cci_value(true) },

                                        { "test_bench.mem1.shared_memory", cci::cci_value(true) },
                                        { "test_bench.pass.mem2.shared_memory", cci::cci_value(true) },
                                        { "test_bench.pass.mem3.shared_memory", cci::cci_value(true) },

                                        { "test_bench.pass.tlm_initiator_ports_num", cci::cci_value(1) },
                                        { "test_bench.pass.tlm_target_ports_num", cci::cci_value(2) },

                                        { "test_bench.pass.remote_pass.tlm_initiator_ports_num", cci::cci_value(2) },
                                        { "test_bench.pass.remote_pass.tlm_target_ports_num", cci::cci_value(1) },

                                        { "test_bench.pass.target_socket_0.address", cci::cci_value(0x20000) },
                                        { "test_bench.pass.target_socket_0.size", cci::cci_value(0x10000) },
                                        { "test_bench.pass.target_socket_0.relative_addresses", cci::cci_value(false) },
                                        { "test_bench.pass.exec_path", cci::cci_value(getexepath() + "-remote") },
                                    });

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}