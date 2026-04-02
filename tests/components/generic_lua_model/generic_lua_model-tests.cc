/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generic_lua_model-test.h"

Test_generic_lua_model::Test_generic_lua_model(const sc_core::sc_module_name& n)
    : TestBench(n)
    , m_memory("memory")
    , m_reg_memory("reg_memory")
    , m_reg_router("reg_router")
    , m_router("router")
    , m_initiator("initiator")
    , m_signal_initiator0("signal_initiator0")
    , m_signal_initiator1("signal_initiator1")
    , m_reset_signal_initiator("reset_out")
    , m_target_signal0("target_signal0")
    , m_target_signal1("target_signal1")
    , reset_counter(0)
{
    m_gen_lua_model = std::make_unique<gs::generic_lua_model>("gen_lua_model", &m_reg_router, &m_reg_memory);
    m_router.initiator_socket.bind(m_gen_lua_model->target_socket);
    m_router.initiator_socket.bind(m_memory.socket);
    m_router.initiator_socket.bind(m_reg_router.target_socket);
    m_reg_router.initiator_socket.bind(m_reg_memory.socket);
    m_initiator.socket.bind(m_router.target_socket);
    m_gen_lua_model->initiator_socket.bind(m_router.target_socket);
    m_signal_initiator0.bind(m_gen_lua_model->target_signal_sockets[0]);
    m_signal_initiator1.bind(m_gen_lua_model->target_signal_sockets[1]);
    m_reset_signal_initiator.bind(m_gen_lua_model->reset);
    m_gen_lua_model->initiator_signal_sockets[0].bind(m_target_signal0);
    m_gen_lua_model->initiator_signal_sockets[1].bind(m_target_signal1);
    m_target_signal0.register_value_changed_cb([&](bool value) {
        SCP_DEBUG(()) << "initiator_signal_sokcet_0 triggered, value: " << std::boolalpha << value;
        ASSERT_EQ(value, false);
    });
    m_target_signal1.register_value_changed_cb([&](bool value) {
        SCP_DEBUG(()) << "initiator_signal_sokcet_1 triggered, value: " << std::boolalpha << value;
        switch (reset_counter) {
        case 1:
            ASSERT_EQ(value, true);
            break;
        case 2:
            ASSERT_EQ(value, false);
            break;
        case 3:
            ASSERT_EQ(value, true);
            break;
        default:
            break;
        }
    });
}

void Test_generic_lua_model::test_reactions()
{
    uint32_t w_value = 0x1UL;
    uint32_t r_value = 0x0UL;
    SCP_DEBUG(()) << "***** Testing the memory write reaction to target signal 0 write with value true *****";
    m_initiator.do_write(0x80000000UL, w_value);
    m_initiator.do_read(0x80000000UL, r_value);
    ASSERT_EQ(r_value, 0x1UL);
    reset_counter++;
    m_signal_initiator0->write(
        true); // there is a target signal 0 write action set to write 0xdeadbeefUL to memory address 0x80000000UL
    wait(2, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x80000000UL, r_value);
    ASSERT_EQ(r_value, 0xdeadbeefUL);

    SCP_DEBUG(()) << "***** Testing the memory write reaction to the same target signal 0 write with value false *****";
    reset_counter++;
    m_signal_initiator0->write(false);
    wait(1, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x80000000UL, r_value);
    ASSERT_EQ(r_value, 0xbeefdeadUL);

    SCP_DEBUG(()) << "***** Testing the memory write reaction to target signal 1 write with value true *****";
    w_value = 0x1UL;
    m_initiator.do_write(0x80000400UL, w_value);
    m_initiator.do_read(0x80000400UL, r_value);
    ASSERT_EQ(r_value, 0x1UL);
    m_signal_initiator1->write(true);
    wait(2, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x80000400UL, r_value);
    ASSERT_EQ(r_value, 0xdeadbeefUL);

    SCP_DEBUG(()) << "***** Testing the register callback reaction to register write *****";
    w_value = 0x12345678UL;
    m_initiator.do_write(0xb80000UL, w_value); // there is a pre_write action set to write 0x7ffffffe to the register
    wait(1, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0xb80000UL, r_value);
    ASSERT_EQ(r_value, 0x10000008UL);
    // post write reaction
    m_initiator.do_read(0x80000200UL, r_value);
    ASSERT_EQ(r_value, 0xababcdcd);

    SCP_DEBUG(()) << "***** Testing the memory data write reaction to register read *****";
    reset_counter++;
    m_initiator.do_read(0xb80004UL, r_value); // there is a post_read action set to write 0xbeefdeadUL to 0x80000004UL
                                              // and write 0x1234abcdUL to 0x80000008UL
    wait(21, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x80000004UL, r_value);
    ASSERT_EQ(r_value, 0xbeefdeadUL);
    wait(20, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x80000008UL, r_value);
    ASSERT_EQ(r_value, 0x1234abcdUL);

    SCP_DEBUG(()) << "***** Testing reaction to register write and read using CCI based if condition *****";
    w_value = 0x1;
    m_initiator.do_write(0x80000600UL, w_value);
    m_initiator.do_write(
        0xb80008UL, w_value); // there is a post_write action set to write 0x24246868UL to 0x80000600UL
                              // if the value written is 0x12345678UL and a pre_read action to mask the
                              // register and set value to 0x11f1aba8UL if the value of the register is 0x12345678UL
    wait(21, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x80000600UL, r_value);
    ASSERT_EQ(r_value, 0x1);
    w_value = 0x12345678UL;
    m_initiator.do_write(0xb80008UL, w_value);
    wait(20, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x80000600UL, r_value);
    ASSERT_EQ(r_value, 0x24246868UL);
    m_initiator.do_read(0xb80008UL, r_value);
    ASSERT_EQ(r_value, 0x11f1aba8UL);

    SCP_DEBUG(()) << "***** Testing the memory write reaction to after_ns *****";
    w_value = 0xFFFFFFFFUL;
    m_initiator.do_write(0x8000000CUL, w_value);
    m_initiator.do_write(0x80000010UL, w_value);
    m_initiator.do_write(0x80000020UL, w_value);
    m_initiator.do_write(0x8000002CUL, w_value);
    m_initiator.do_read(0x8000000CUL, r_value);
    ASSERT_EQ(r_value, 0xFFFFFFFFUL);
    m_initiator.do_read(0x80000010UL, r_value);
    ASSERT_EQ(r_value, 0xFFFFFFFFUL);
    m_initiator.do_read(0x80000020UL, r_value);
    ASSERT_EQ(r_value, 0xFFFFFFFFUL);
    m_initiator.do_read(0x8000002CUL, r_value);
    ASSERT_EQ(r_value, 0xFFFFFFFFUL);
    // after 201 ns
    sc_core::wait(201, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x8000000CUL, r_value);
    ASSERT_EQ(r_value, 0xabcd1234UL);
    m_initiator.do_read(0x80000010UL, r_value);
    ASSERT_EQ(r_value, 0x56784321UL);

    w_value = 0x0UL;
    m_initiator.do_write(0x8000000CUL, w_value);
    m_initiator.do_write(0x80000010UL, w_value);
    m_initiator.do_read(0x8000000CUL, r_value);
    ASSERT_EQ(r_value, 0x0UL);
    m_initiator.do_read(0x80000010UL, r_value);
    ASSERT_EQ(r_value, 0x0UL);

    SCP_DEBUG(()) << "********** assert reset signal ********************";
    m_reset_signal_initiator->write(true);

    // The values of the memory locations which should have been set after 400 ns are not set
    // because a reset happens before simulation time reaches to 400 ns, so these memory locations
    // will be written to the configured values after 400 ns from the time the reset signal was asserted.
    m_initiator.do_read(0x80000020UL, r_value);
    ASSERT_EQ(r_value, 0xFFFFFFFFUL);
    m_initiator.do_read(0x8000002CUL, r_value);
    ASSERT_EQ(r_value, 0xFFFFFFFFUL);

    w_value = 0x0UL;
    m_initiator.do_write(0x80000020UL, w_value);
    m_initiator.do_write(0x8000002CUL, w_value);
    m_initiator.do_read(0x80000020UL, r_value);
    ASSERT_EQ(r_value, 0x0UL);
    m_initiator.do_read(0x8000002CUL, r_value);
    ASSERT_EQ(r_value, 0x0UL);

    // wait 201 ns
    sc_core::wait(201, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x8000000CUL, r_value);
    ASSERT_EQ(r_value, 0xabcd1234UL);
    m_initiator.do_read(0x80000010UL, r_value);
    ASSERT_EQ(r_value, 0x56784321UL);

    // after 401 ns
    sc_core::wait(200, sc_core::sc_time_unit::SC_NS);
    m_initiator.do_read(0x80000020UL, r_value);
    ASSERT_EQ(r_value, 0x1234abcdUL);
    m_initiator.do_read(0x8000002CUL, r_value);
    ASSERT_EQ(r_value, 0x43215678UL);
}

TEST_BENCH(Test_generic_lua_model, Tester)
{
    SCP_DEBUG(()) << "***** Start generic_lua_model Testing *****";
    test_reactions();
}

int sc_main(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(50));      // make the msg type column a bit tighter
    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig{ "sc_main" };
    auto broker_h = m_broker.create_broker_handle(orig);
    ArgParser ap{ broker_h, argc, argv };
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
