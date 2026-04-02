/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "container_builder-bench.h"
#include <tests/initiator-tester.h>

ContainerBuilderTestBench::ContainerBuilderTestBench(const sc_core::sc_module_name& n): TestBench(n)
{
    m_platform = std::make_unique<Container>("platform");
}

TEST_BENCH(ContainerBuilderTestBench, AllTests)
{
    ASSERT_TRUE(m_platform != nullptr) << "Failed to create platform";

    InitiatorTester* initiator = dynamic_cast<InitiatorTester*>(sc_core::sc_find_object("AllTests.platform.initiator"));
    ASSERT_TRUE(initiator != nullptr) << "Failed to find main initiator";

    InitiatorTester* initiator1 = dynamic_cast<InitiatorTester*>(
        sc_core::sc_find_object("AllTests.platform.container1.initiator1"));
    ASSERT_TRUE(initiator1 != nullptr) << "Failed to find container1 initiator1";

    InitiatorTester* initiator2 = dynamic_cast<InitiatorTester*>(
        sc_core::sc_find_object("AllTests.platform.container2.initiator2"));
    ASSERT_TRUE(initiator2 != nullptr) << "Failed to find container2 initiator2";

    InitiatorTester* initiator_nested = dynamic_cast<InitiatorTester*>(
        sc_core::sc_find_object("AllTests.platform.container1.nested_container.initiator_nested"));
    ASSERT_TRUE(initiator_nested != nullptr) << "Failed to find nested initiator";

    uint32_t write_data, read_data;
    tlm::tlm_response_status status;

    write_data = 0xDEADBEEF;
    status = initiator->do_write<uint32_t>(0x10000000, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Write to memory1 failed";

    read_data = 0;
    status = initiator->do_read<uint32_t>(0x10000000, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Read from memory1 failed";
    ASSERT_EQ(read_data, write_data) << "Memory1 read/write mismatch";

    write_data = 0xCAFEBABE;
    status = initiator->do_write<uint32_t>(0x20000000, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Write to memory2 failed";

    read_data = 0;
    status = initiator->do_read<uint32_t>(0x20000000, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Read from memory2 failed";
    ASSERT_EQ(read_data, write_data) << "Memory2 read/write mismatch";

    write_data = 0x12345678;
    status = initiator->do_write<uint32_t>(0x30000000, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Write to memory3 failed";

    read_data = 0;
    status = initiator->do_read<uint32_t>(0x30000000, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Read from memory3 failed";
    ASSERT_EQ(read_data, write_data) << "Memory3 read/write mismatch";

    write_data = 0xAABBCCDD;
    status = initiator1->do_write<uint32_t>(0x40000000, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Write to container1.memory_a failed";

    read_data = 0;
    status = initiator1->do_read<uint32_t>(0x40000000, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Read from container1.memory_a failed";
    ASSERT_EQ(read_data, write_data) << "Container1.memory_a read/write mismatch";

    write_data = 0x11223344;
    status = initiator1->do_write<uint32_t>(0x50000000, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Write to container1.memory_b failed";

    read_data = 0;
    status = initiator1->do_read<uint32_t>(0x50000000, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Read from container1.memory_b failed";
    ASSERT_EQ(read_data, write_data) << "Container1.memory_b read/write mismatch";

    write_data = 0x55667788;
    status = initiator_nested->do_write<uint32_t>(0x60000000, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Write to nested_container.memory_nested failed";

    read_data = 0;
    status = initiator_nested->do_read<uint32_t>(0x60000000, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Read from nested_container.memory_nested failed";
    ASSERT_EQ(read_data, write_data) << "Nested_container.memory_nested read/write mismatch";

    write_data = 0x99AABBCC;
    status = initiator2->do_write<uint32_t>(0x70000000, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Write to container2.memory_x failed";

    read_data = 0;
    status = initiator2->do_read<uint32_t>(0x70000000, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Read from container2.memory_x failed";
    ASSERT_EQ(read_data, write_data) << "Container2.memory_x read/write mismatch";

    write_data = 0xDDEEFF00;
    status = initiator2->do_write<uint32_t>(0x80000000, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Write to container2.memory_y failed";

    read_data = 0;
    status = initiator2->do_read<uint32_t>(0x80000000, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Read from container2.memory_y failed";
    ASSERT_EQ(read_data, write_data) << "Container2.memory_y read/write mismatch";

    write_data = 0xABCD0123;
    status = initiator->do_write<uint32_t>(0x10000004, write_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Second write to memory1 failed";

    read_data = 0;
    status = initiator->do_read<uint32_t>(0x10000004, read_data);
    ASSERT_EQ(status, tlm::TLM_OK_RESPONSE) << "Second read from memory1 failed";
    ASSERT_EQ(read_data, write_data) << "Memory1 second read/write mismatch";

    sc_core::sc_object* container1_obj = sc_core::sc_find_object("AllTests.platform.container1");
    ASSERT_TRUE(container1_obj != nullptr) << "container1 not found";

    sc_core::sc_object* container2_obj = sc_core::sc_find_object("AllTests.platform.container2");
    ASSERT_TRUE(container2_obj != nullptr) << "container2 not found";

    sc_core::sc_object* router1 = sc_core::sc_find_object("AllTests.platform.container1.router");
    ASSERT_TRUE(router1 != nullptr) << "container1.router not created from config - .config parameter not working!";

    sc_core::sc_object* router2 = sc_core::sc_find_object("AllTests.platform.container2.router");
    ASSERT_TRUE(router2 != nullptr) << "container2.router not created from config - .config parameter not working!";

    sc_core::sc_object* memory_a = sc_core::sc_find_object("AllTests.platform.container1.memory_a");
    ASSERT_TRUE(memory_a != nullptr) << "container1.memory_a not created from config";

    sc_core::sc_object* memory_x = sc_core::sc_find_object("AllTests.platform.container2.memory_x");
    ASSERT_TRUE(memory_x != nullptr) << "container2.memory_x not created from config";
}

int sc_main(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::INFO)
                          .msgTypeFieldWidth(50));

    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig{ "sc_main" };
    auto broker_h = m_broker.create_broker_handle(orig);

    ArgParser ap{ broker_h, argc, argv };

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
