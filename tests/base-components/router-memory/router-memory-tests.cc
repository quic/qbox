/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include "router-memory-bench.h"
#include <tests/target-tester.h>
#include <cci/utils/broker.h>

// Simple read into the memory
TEST_BENCH(RouterMemoryTestBench, SimpleWriteRead)
{
    uint8_t data;
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write(0, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write(address[1], 0x08), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(address[1], data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x08);

    /* Target 3 */
    ASSERT_EQ(m_initiator.do_write(address[3], 0x08), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(address[3], data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x08);
}

// Transaction outside of the target address space
TEST_BENCH(RouterMemoryTestBench, SimpleOverlapWrite)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(memory_size[0], 0x04), tlm::TLM_OK_RESPONSE);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(memory_size[1], 0x08), tlm::TLM_OK_RESPONSE);
}

// Transaction that crosses the boundary
TEST_BENCH(RouterMemoryTestBench, SimpleCrossesBoundary)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(memory_size[0] - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(memory_size[1] - 1, 0xFFFF), tlm::TLM_ADDRESS_ERROR_RESPONSE);
}

// Simple write and read into the memory with the Debug Transport Interface
TEST_BENCH(RouterMemoryTestBench, SimpleWriteReadDebug)
{
    uint8_t data8 = 0;
    uint16_t data16 = 0;
    uint32_t data32 = 0;
    uint64_t data64 = 0;

    // set some data so that we are sure we only get the right amount below
    ASSERT_EQ(m_initiator.do_write<uint64_t>(0, -1, true), tlm ::TLM_OK_RESPONSE);

    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data8, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data8));
    ASSERT_EQ(data8, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint16_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint32_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint32_t));
    ASSERT_EQ(m_initiator.do_read(0, data32, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data32));
    ASSERT_EQ(data32, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint64_t>(0, 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint64_t));
    ASSERT_EQ(m_initiator.do_read(0, data64, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data64));
    ASSERT_EQ(data64, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(address[1], 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data8, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data8));
    ASSERT_EQ(data8, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint16_t>(address[1], 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint16_t));
    ASSERT_EQ(m_initiator.do_read(0, data16, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data16));
    ASSERT_EQ(data16, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint32_t>(address[1], 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint32_t));
    ASSERT_EQ(m_initiator.do_read(0, data32, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data32));
    ASSERT_EQ(data32, 0x04);

    ASSERT_EQ(m_initiator.do_write<uint64_t>(address[1], 0x04, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint64_t));
    ASSERT_EQ(m_initiator.do_read(0, data64, true), tlm ::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data64));
    ASSERT_EQ(data64, 0x04);

    ASSERT_EQ(m_initiator.do_write(0x10000, data32), tlm::TLM_OK_RESPONSE);
}

// Debug Transport Interface transaction outside of the target address space
TEST_BENCH(RouterMemoryTestBench, SimpleOverlapWriteDebug)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(memory_size[0], 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_NE(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(memory_size[1], 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_NE(m_initiator.get_last_transport_debug_ret(), 0);
}

// Debug Transport Interface transaction that crosses the boundary
TEST_BENCH(RouterMemoryTestBench, SimpleCrossesBoundaryDebug)
{
    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(memory_size[0] - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint16_t>(memory_size[1] - 1, 0xFFFF, true), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), 0);
}

// Write into memory with Blocking Transport and read with Debug Transport Interface
TEST_BENCH(RouterMemoryTestBench, WriteBlockingReadDebug)
{
    uint8_t data;

    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(0, data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(address[1], 0x04), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.do_read(address[1], data, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(data));
    ASSERT_EQ(data, 0x04);

    gs::memorydumper_tgr_helper();
}

// Write into memory with Debug Transport Interface and read with Blocking Transport
TEST_BENCH(RouterMemoryTestBench, WriteDebugReadBlocking)
{
    uint8_t data;

    /* Target 1 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(0, 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(0, data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);

    /* Target 2 */
    ASSERT_EQ(m_initiator.do_write<uint8_t>(address[1], 0x04, true), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(m_initiator.get_last_transport_debug_ret(), sizeof(uint8_t));
    ASSERT_EQ(m_initiator.do_read(address[1], data), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 0x04);
}

// Request for DMI access to memory
TEST_BENCH(RouterMemoryTestBench, SimpleDmi)
{
    /*
     * Memory layout (all memories are size 0x1000):
     *   Memory_0: address=0x0000, size=0x1000, priority=0 -> [0x0000, 0x1000)
     *   Memory_1: address=0x0200, size=0x1000, priority=1 -> [0x0200, 0x1200)
     *   Memory_2: address=0x0400, size=0x1000, priority=2 -> [0x0400, 0x1400)
     *   Memory_3: address=0x0600, size=0x1000, priority=3 -> [0x0600, 0x1600)
     *
     * After priority-based splitting (lower priority value wins):
     *   [0x0000, 0x0200) -> Memory_0
     *   [0x0200, 0x0400) -> Memory_0
     *   [0x0400, 0x0600) -> Memory_0
     *   [0x0600, 0x1000) -> Memory_0
     *   [0x1000, 0x1200) -> Memory_1
     *   [0x1200, 0x1400) -> Memory_2
     *   [0x1400, 0x1600) -> Memory_3
     *
     * DMI regions must respect segment boundaries so they don't span
     * across a region owned by a different target.
     */

    /* Address 0x0 falls into Memory 0's split segment [0x0, 0x200) */
    do_good_dmi_request_and_check(address[0], address[0], address[1] - 1);

    /* Address 0x1000 falls into Memory 1's split segment [0x1000, 0x1200) */
    do_good_dmi_request_and_check(memory_size[0], memory_size[0], memory_size[1] - 1);

    /* Address 0x200 falls into Memory 0's split segment [0x200, 0x400) */
    do_good_dmi_request_and_check(address[1], address[1], address[2] - 1);

    /* Address 0x1200 falls into Memory 2's split segment [0x1200, 0x1400) */
    do_good_dmi_request_and_check(memory_size[1], memory_size[1], memory_size[2] - 1);

    // Add a check for a truly out-of-bounds address, e.g., beyond the last memory
    // Max address in current config is 0x600 + 0x1000 - 1 = 0x15ff
    do_bad_dmi_request_and_check(0x2000);
}

// Write and Read into the mémory with the Direct Memory Interface
TEST_BENCH(RouterMemoryTestBench, DmiWriteRead)
{
    uint8_t data = 0x04;
    uint8_t data_read;

    /* Address 0x50 falls into Memory 0's split segment [0x0, 0x200) */
    do_good_dmi_request_and_check(address[0] + 0x50, address[0], address[1] - 1);
    /* Write with DMI target N°0 */
    dmi_write_or_read(0x50, &data, sizeof(data), false);
    /* Read with DMI target N°0 */
    dmi_write_or_read(0x50, &data_read, sizeof(data), true);
    ASSERT_EQ(data, data_read);

    /* Address 0x250 falls into Memory 0's split segment [0x200, 0x400) */
    do_good_dmi_request_and_check(address[1] + 0x50, address[1], address[2] - 1);
    /* Write with DMI target N°0 */
    dmi_write_or_read(address[1] + 0x50, &data, sizeof(data), false);
    /* Read with DMI target N°0 */
    dmi_write_or_read(address[1] + 0x50, &data_read, sizeof(data), true);
    ASSERT_EQ(data, data_read);
}

/*
 * Test bench for the common case: a low-priority memory with a higher-priority
 * non-DMI target layered on top. The DMI regions for the memory must not span
 * across the non-DMI target's address range.
 *
 *   Memory:  address=0x0000, size=0x1000, priority=1 -> [0x0000, 0x1000) (grants DMI)
 *   Device:  address=0x0400, size=0x0100, priority=0 -> [0x0400, 0x0500) (no DMI)
 *
 * After priority-based splitting:
 *   [0x0000, 0x0400) -> Memory
 *   [0x0400, 0x0500) -> Device
 *   [0x0500, 0x1000) -> Memory
 */
class OverlayNoDmiTestBench : public TestBench
{
public:
    static constexpr uint64_t MEM_ADDR = 0x0;
    static constexpr uint64_t MEM_SIZE = 0x1000;
    static constexpr uint64_t DEV_ADDR = 0x400;
    static constexpr uint64_t DEV_SIZE = 0x100;

protected:
    InitiatorTester m_initiator;
    gs::router<> m_router;
    gs::gs_memory<> m_memory;
    TargetTester m_device;

    void do_good_dmi_request_and_check(uint64_t addr, uint64_t exp_start, uint64_t exp_end)
    {
        bool ret = m_initiator.do_dmi_request(addr);
        const tlm::tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();

        ASSERT_TRUE(ret);
        ASSERT_EQ(exp_start, dmi_data.get_start_address());
        ASSERT_EQ(exp_end, dmi_data.get_end_address());
    }

    void do_bad_dmi_request_and_check(uint64_t addr)
    {
        bool ret = m_initiator.do_dmi_request(addr);
        ASSERT_FALSE(ret);
    }

    void dmi_write_or_read(uint64_t addr, uint8_t* data, size_t len, bool is_read)
    {
        const tlm::tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();
        addr -= dmi_data.get_start_address();

        if (is_read) {
            ASSERT_TRUE(dmi_data.is_read_allowed());
            memcpy(data, dmi_data.get_dmi_ptr() + addr, len);
        } else {
            ASSERT_TRUE(dmi_data.is_write_allowed());
            memcpy(dmi_data.get_dmi_ptr() + addr, data, len);
        }
    }

public:
    OverlayNoDmiTestBench(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_initiator("initiator")
        , m_router("router")
        , m_memory("Memory", MEM_SIZE)
        , m_device("Device", DEV_SIZE)
    {
        m_router.add_initiator(m_initiator.socket);
        m_router.add_target(m_memory.socket, MEM_ADDR, MEM_SIZE, true, /*priority=*/1);
        m_router.add_target(m_device.socket, DEV_ADDR, DEV_SIZE, true, /*priority=*/0);
    }
};

// DMI regions must stop at the non-DMI device's boundaries
TEST_BENCH(OverlayNoDmiTestBench, DmiRegionClippedByHigherPriorityNoDmiTarget)
{
    /* DMI before the device: segment [0x0, 0x400) */
    do_good_dmi_request_and_check(0x0, 0x0, DEV_ADDR - 1);
    do_good_dmi_request_and_check(0x3FF, 0x0, DEV_ADDR - 1);

    /* DMI in the device region: must be denied */
    do_bad_dmi_request_and_check(DEV_ADDR);
    do_bad_dmi_request_and_check(DEV_ADDR + DEV_SIZE / 2);

    /* DMI after the device: segment [0x500, 0x1000) */
    do_good_dmi_request_and_check(DEV_ADDR + DEV_SIZE, DEV_ADDR + DEV_SIZE, MEM_SIZE - 1);
    do_good_dmi_request_and_check(0xFFF, DEV_ADDR + DEV_SIZE, MEM_SIZE - 1);
}

// Write and read via DMI on both sides of the non-DMI device
TEST_BENCH(OverlayNoDmiTestBench, DmiWriteReadAroundNoDmiTarget)
{
    uint8_t data, data_read;

    /* Write/read in the region before the device */
    do_good_dmi_request_and_check(0x50, 0x0, DEV_ADDR - 1);
    data = 0xAA;
    dmi_write_or_read(0x50, &data, sizeof(data), false);
    dmi_write_or_read(0x50, &data_read, sizeof(data_read), true);
    ASSERT_EQ(data, data_read);

    /* Write/read in the region after the device */
    do_good_dmi_request_and_check(0x600, DEV_ADDR + DEV_SIZE, MEM_SIZE - 1);
    data = 0xBB;
    dmi_write_or_read(0x600, &data, sizeof(data), false);
    dmi_write_or_read(0x600, &data_read, sizeof(data_read), true);
    ASSERT_EQ(data, data_read);
}

int sc_main(int argc, char* argv[])
{
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
