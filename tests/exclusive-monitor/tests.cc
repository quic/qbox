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

#include "test-bench.h"
#include <cci/utils/broker.h>

/*
 * Regular load and stores. Check that the monitor does not introduce bugs when
 * no exclusive transaction are in use.
 */
TEST_BENCH(ExclusiveMonitorTestBench, SimpleReadWrite)
{
    std::cout << "TEST_BENCH: SimpleReadWrite\n";
    /* Normal load/store at address 0 */
    do_load_and_check(0, 8);
    do_store_and_check(0, 8);

    /* Out-of-bound load/store */
    do_load_and_check(TARGET_MMIO_SIZE, 8);
    do_store_and_check(TARGET_MMIO_SIZE, 8);
}

/*
 * Same as previous test, with DMI requests
 */
TEST_BENCH(ExclusiveMonitorTestBench, SimpleDmi)
{
    std::cout << "TEST_BENCH: SimpleDmi\n";
    /* Valid DMI request */
    do_good_dmi_request_and_check(0, 0, TARGET_MMIO_SIZE - 1);

    /* Out-of-bound DMI request */
    do_bad_dmi_request_and_check(TARGET_MMIO_SIZE);
}

/*
 * Basic exclusive load/store pair, same address, same ID. Expect the load to
 * trigger a DMI invalidation and the store to succeed.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLdExclSt)
{
    std::cout << "TEST_BENCH: ExclLdExclSt\n";
    do_good_dmi_request_and_check(0, 0, TARGET_MMIO_SIZE - 1);
    do_excl_load_and_check(0, 0, 8, true);
    do_excl_store_and_check(0, 0, 8, true);
}

/*
 * Out-of-bound exclusive pair. The load should fail normally (with the target
 * returning a TLM address error), and the store should fail because the region
 * was not locked by the load.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLdExclStOutOfBound)
{
    std::cout << "TEST_BENCH: ExclLdExclStOutOfBound\n";
    do_excl_load_and_check(0, TARGET_MMIO_SIZE, 8, false);
    do_excl_store_and_check(0, TARGET_MMIO_SIZE, 8, false);
}

/*
 * Mismatch between the load and store address. The store should fail.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLdExclStAddrMismatch)
{
    std::cout << "TEST_BENCH: ExclLdExclStAddrMismatch\n";
    do_excl_load_and_check(0, 0, 8, true);
    do_excl_store_and_check(0, 0x20, 8, false);
}

/*
 * The store is not aligned with the load. The store should fail.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLdExclStUnaligned)
{
    std::cout << "TEST_BENCH: ExclLdExclStUnaligned\n";
    do_excl_load_and_check(0, 0, 8, true);
    do_excl_store_and_check(0, 4, 8, false);
}

/*
 * The store is smaller than the load. The store should fail.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLdExclStSizeMismatch0)
{
    std::cout << "TEST_BENCH: ExclLdExclStSizeMismatch0\n";
    do_excl_load_and_check(0, 0, 8, true);
    do_excl_store_and_check(0, 0, 4, false);
}

/*
 * The store is bigger than the load. The store should fail.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLdExclStSizeMismatch1)
{
    std::cout << "TEST_BENCH: ExclLdExclStSizeMismatch1\n";
    do_excl_load_and_check(0, 0, 8, true);
    do_excl_store_and_check(0, 0, 16, false);
}

/*
 * Store without preceding load, it should fail.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclStAlone)
{
    std::cout << "TEST_BENCH: ExclStAlone\n";
    do_excl_store_and_check(0, 0, 8, false);
}

/*
 * Exclusive load/store pair with a standard load in between. The exclusive
 * store should succeed.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLdStandardLdExclSt)
{
    std::cout << "TEST_BENCH: ExclLdStandardLdExclSt\n";
    do_good_dmi_request_and_check(0, 0, TARGET_MMIO_SIZE - 1);
    do_excl_load_and_check(0, 0, 8, true);
    do_load_and_check(0, 8);
    do_excl_store_and_check(0, 0, 8, true);
}

/*
 * Exclusive load/store pair with a regular store in between. The exclusive
 * store should fail.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLdStandardStExclSt)
{
    std::cout << "TEST_BENCH: ExclLdStandardStExclSt\n";
    do_good_dmi_request_and_check(0, 0, TARGET_MMIO_SIZE - 1);
    do_excl_load_and_check(0, 0, 8, true);
    do_store_and_check(0, 8);
    do_excl_store_and_check(0, 0, 8, false);
}

/*
 * Multiple Exclusive load/store pair with a regular store in between covering
 * some of them. The exclusive store that are covered by the regular store
 * should fail.
 */
TEST_BENCH(ExclusiveMonitorTestBench, MultipleExclLdStandardStExclSt)
{
    std::cout << "TEST_BENCH: MultipleExclLdStandardStExclSt\n";
    do_good_dmi_request_and_check(0, 0, TARGET_MMIO_SIZE - 1);

    do_excl_load_and_check(0, 0, 8, true);
    do_excl_load_and_check(1, 8, 8, true);
    do_excl_load_and_check(2, 16, 8, true);
    do_excl_load_and_check(3, 24, 8, true);

    /* Covers from 9 to 20 */
    do_store_and_check(9, 20 - 9 + 1);

    do_excl_store_and_check(0, 0, 8, true);
    do_excl_store_and_check(1, 8, 8, false);
    do_excl_store_and_check(2, 16, 8, false);
    do_excl_store_and_check(3, 24, 8, true);
}

/*
 * Two Exclusive pairs at two different addresses with two different IDs. Both
 * should succeed.
 */
TEST_BENCH(ExclusiveMonitorTestBench, TwoDistinctExclPairs)
{
    std::cout << "TEST_BENCH: TwoDistinctExclPairs\n";
    do_excl_load_and_check(0, 0, 8, true);
    do_excl_load_and_check(1, 0x20, 8, true);
    do_excl_store_and_check(0, 0, 8, true);
    do_excl_store_and_check(1, 0x20, 8, true);
}

/*
 * Two Exclusive pairs at the same address with two different IDs. The second
 * load should not trigger a DMI invalidation and the second store should fail.
 */
TEST_BENCH(ExclusiveMonitorTestBench, TwoDistinctExclPairsSameAddr)
{
    std::cout << "TEST_BENCH: TwoDistinctExclPairsSameAddr\n";
    do_excl_load_and_check(0, 0, 8, true);
    do_excl_load_and_check(1, 0, 8, false);
    do_excl_store_and_check(0, 0, 8, true);
    do_excl_store_and_check(1, 0, 8, false);
}

/*
 * Two Exclusive pairs at two different addresses but with the same ID. Both
 * loads should trigger a DMI invalidation and only the second store should
 * succeed.
 */
TEST_BENCH(ExclusiveMonitorTestBench, TwoDistinctExclPairsSameId)
{
    std::cout << "TEST_BENCH: TwoDistinctExclPairsSameId\n";
    do_excl_load_and_check(0, 0, 8, true);
    do_excl_load_and_check(0, 0x20, 8, true);
    do_excl_store_and_check(0, 0, 8, false);
    do_excl_store_and_check(0, 0x20, 8, true);
}

/*
 * Exclusive reservations should split the DMI space to skip the locked
 * regions. This test exercises this by locking/unlocking regions and doing DMI
 * requests accordingly.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLockDmiReq)
{
    std::cout << "TEST_BENCH: ExclLockDmiReq\n";
    /* First request should return the whole range */
    do_good_dmi_request_and_check(0, 0, TARGET_MMIO_SIZE - 1);

    /* First locking at address 128, should split the DMI space in two */
    do_excl_load_and_check(0, 128, 8, true);

    /* We should now have two distinct ranges */
    do_good_dmi_request_and_check(0, 0, 128 - 1);
    do_good_dmi_request_and_check(124, 0, 128 - 1);
    do_bad_dmi_request_and_check(128);
    do_bad_dmi_request_and_check(132);
    do_good_dmi_request_and_check(512, 128 + 8, TARGET_MMIO_SIZE - 1);

    /* Add another exclusive region at address 640 */
    do_excl_load_and_check(1, 640, 8, true);

    /* We should now have three distinct ranges */
    do_good_dmi_request_and_check(0, 0, 128 - 1);
    do_bad_dmi_request_and_check(128);
    do_good_dmi_request_and_check(512, 128 + 8, 640 - 1);
    do_bad_dmi_request_and_check(640);
    do_good_dmi_request_and_check(800, 640 + 8, TARGET_MMIO_SIZE - 1);

    /* Invalidate the first region */
    do_excl_store_and_check(0, 128, 8, true);

    /* We should be back with two ranges */
    do_good_dmi_request_and_check(0, 0, 640 - 1);
    do_bad_dmi_request_and_check(640);
    do_good_dmi_request_and_check(800, 640 + 8, TARGET_MMIO_SIZE - 1);

    /* Invalidate the second region */
    do_excl_store_and_check(1, 640, 8, true);

    /* We should see the whole range again */
    do_good_dmi_request_and_check(0, 0, TARGET_MMIO_SIZE - 1);
}

/*
 * This test is similar with the previous one, but checks the DMI hint value
 * returned by transactions.
 */
TEST_BENCH(ExclusiveMonitorTestBench, ExclLockDmiHint)
{
    std::cout << "TEST_BENCH: ExclLockDmiHint\n";
    /* Regular load */
    do_load_and_check(0, 8);
    ASSERT_TRUE(get_last_dmi_hint());

    /* Regular store */
    do_store_and_check(0, 8);
    ASSERT_TRUE(get_last_dmi_hint());

    /* Lock a region, hint should be false */
    do_excl_load_and_check(0, 128, 8, true);
    ASSERT_FALSE(get_last_dmi_hint(0));

    /* Unlock it, hint should be true */
    do_excl_store_and_check(0, 128, 8, true);
    ASSERT_TRUE(get_last_dmi_hint(0));

    /* Relock the region, hint should be false */
    do_excl_load_and_check(0, 128, 8, true);
    ASSERT_FALSE(get_last_dmi_hint(0));

    /*
     * Already locked, the exclusive load has no effect but the hint should
     * still be false.
     */
    do_excl_load_and_check(0, 132, 8, false);
    ASSERT_FALSE(get_last_dmi_hint(0));

    /* Regular load, region locked, hint should be false */
    do_load_and_check(124, 16);
    ASSERT_FALSE(get_last_dmi_hint());

    /* Regular load, region unlocked, hint should be true */
    do_load_and_check(64, 8);
    ASSERT_TRUE(get_last_dmi_hint());

    /* Regular load, region unlocked, hint should be true */
    do_load_and_check(512, 8);
    ASSERT_TRUE(get_last_dmi_hint());

    /* Regular store, the region get unlocked, hint should be true */
    do_store_and_check(124, 16);
    ASSERT_TRUE(get_last_dmi_hint());
}

int sc_main(int argc, char* argv[])
{
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
