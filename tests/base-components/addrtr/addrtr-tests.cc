/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libgsutils.h>
#include "addrtr-bench.h"

/*
 * Regular load and stores. Check that the monitor does not introduce bugs when
 * no exclusive transaction are in use.
 */
TEST_BENCH(AddrtrTestBench, simlpletxn) { do_txn(0, 100, 0); }
TEST_BENCH(AddrtrTestBench, dbgtxn) { do_txn(1, 100, 1); }
TEST_BENCH(AddrtrTestBench, dmiinv) { do_dmi(2, 100); }
int sc_main(int argc, char* argv[])
{
    auto m_broker = new gs::ConfigurableBroker();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
