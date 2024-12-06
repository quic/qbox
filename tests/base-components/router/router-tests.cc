/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include "router-bench.h"
#include <cci/utils/broker.h>

// Simple load and store into the Target 1 and 2
TEST_BENCH(RouterTestBenchSimple, SimpleReadWrite)
{
    /* Normal load/store of Target 1 at address 0 */
    do_load_and_check(0, 0, 8);
    do_store_and_check(0, 0, 8);

    /* Normal load/store of Target 2 at address 270 */
    do_store_and_check(1, 270, 8);
    do_store_and_check(1, 270, 8);

    /* Out-of-bound load/store of Target 1 */
    do_load_and_check(0, target_size[0], 2);
    do_store_and_check(0, target_size[0], 2);

    /* Out-of-bound load/store of Target 2 */
    do_load_and_check(1, target_size[1], 2);
    do_store_and_check(1, target_size[1], 2);

    /* Overlap load/store of Target 1 */
    do_load_and_check(0, target_size[0] - 1, 2);
    do_store_and_check(0, target_size[0] - 1, 2);

    /* Out-of-bound load/store of Target 2 */
    do_load_and_check(1, target_size[1] - 1, 2);
    do_store_and_check(1, target_size[1] - 1, 2);
}

// Simple load and store between two overlapping targets
TEST_BENCH(RouterTestBenchSimple, SimpleReadWriteOverlap)
{
    /* Target 3 overlapping Target 4, we expect an error message */
    do_load_and_check(2, target_size[2] - 1, 1);
    do_store_and_check(2, target_size[2] - 1, 1);

    /* Target 4 overlapping Target 3, we expect an error message */
    do_load_and_check(3, target_size[3] - 1, 1);
    do_store_and_check(3, target_size[3] - 1, 1);
}

// Simple load and store with the Debug Transport Interface into the Target 1 and 2
TEST_BENCH(RouterTestBenchSimple, SimpleReadWriteDebug)
{
    /* Normal load/store of Target 1 at address 0 */
    do_load_and_check(0, 0, 8, true);
    do_store_and_check(0, 0, 8, true);

    /* Normal load/store at address 270 */
    do_load_and_check(1, 270, 8, true);
    do_store_and_check(1, 270, 8, true);

    /* Out-of-bound load/store of Target 1*/
    do_load_and_check(0, target_size[0], 8, true);
    do_store_and_check(0, target_size[0], 8, true);

    /* Out-of-bound load/store of Target 2*/
    do_load_and_check(1, target_size[1], 8, true);
    do_store_and_check(1, target_size[1], 8, true);

    /* Overlap load/store of Target 1*/
    do_load_and_check(0, target_size[0] - 1, 2, true);
    do_store_and_check(0, target_size[0] - 1, 2, true);

    /* Out-of-bound load/store of Target 2*/
    do_load_and_check(1, target_size[1] - 1, 2, true);
    do_store_and_check(1, target_size[1] - 1, 2, true);
}

// Simple load and store between two overlapping targets that use Debug Transport Interface
TEST_BENCH(RouterTestBenchSimple, SimpleReadWriteOverlapDebug)
{
    /* Target 3 overlapping Target 4, we expect an error message */
    do_load_and_check(2, target_size[2] - 1, 1, true);
    do_store_and_check(2, target_size[2] - 1, 1, true);

    /* Target 4 overlapping Target 3, we expect an error message */
    do_load_and_check(3, target_size[3] - 1, 1, true);
    do_store_and_check(3, target_size[3] - 1, 1, true);
}

// Request for DMI access to Target 1 and 2
TEST_BENCH(RouterTestBenchSimple, SimpleDmi)
{
    /* Valid DMI request of Target 1*/
    do_good_dmi_request_and_check(0, 0, 0, target_size[0] - 1);

    /* Valid DMI request of Target 2*/
    do_good_dmi_request_and_check(1, address[1], address[1], target_size[1] - 1);

    /* Out-of-bound DMI request of Target 1*/
    do_bad_dmi_request_and_check(0, target_size[0]);

    /* Out-of-bound DMI request of Target 2*/
    do_bad_dmi_request_and_check(1, target_size[1]);
}

// Simple DMI Overlap
TEST_BENCH(RouterTestBenchSimple, SimpleDmiOverlap)
{
    /* Target 4 overlapping Target 3, we expect an error message */
    do_good_dmi_request_and_check(3, address[3], address[3], target_size[3] - 1);
}

int sc_main(int argc, char* argv[])
{
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}