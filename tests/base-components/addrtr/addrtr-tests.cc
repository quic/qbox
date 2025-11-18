/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <libgsutils.h>
#include "addrtr-bench.h"

/*
 * Regular load and stores. Check that the monitor does not introduce bugs when
 * no exclusive transaction are in use.
 */
TEST_BENCH(AddrtrTestBench, Tester)
{
    do_txn(0x100ULL, 0);
    do_txn(0x100ULL, 1);
    do_dmi(0x100ULL);
}

int sc_main(int argc, char* argv[])
{
    gs::ConfigurableBroker m_broker({
        { "log_level", cci::cci_value(5) },
        { "Tester.exclusive_addrtr.target_socket.address", cci::cci_value(0x100ULL) },
        { "Tester.exclusive_addrtr.target_socket.size", cci::cci_value(AddrtrTestBench::TARGET_MMIO_SIZE) },
        { "Tester.exclusive_addrtr.mapped_base_addr", cci::cci_value(0x110ULL) },
    });

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
