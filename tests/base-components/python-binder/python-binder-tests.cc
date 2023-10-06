#include "python-binder-bench.h"
#include <cci/utils/broker.h>

TEST_BENCH(PythonBinderTestBench, test_bench)
{
    uint8_t w_data[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    uint8_t r_data1[8];
    memset(r_data1, 0, sizeof(r_data1) / sizeof(uint8_t));
    uint8_t r_data2[16];
    memset(r_data2, 0, sizeof(r_data2) / sizeof(uint8_t));

    sc_core::sc_time delay(0, sc_core::sc_time_unit::SC_NS);

    SCP_INFO(()) << "Test basic b_transport" << std::endl;
    tlm::tlm_generic_payload trans;
    print_dashes();
    // write to  python-binder
    do_basic_trans_check(trans, delay, 0x2000, w_data, nullptr, 8, tlm::tlm_command::TLM_WRITE_COMMAND);
    print_dashes();
    // write to  python-binder
    do_basic_trans_check(trans, delay, 0x2100, w_data, nullptr, 8, tlm::tlm_command::TLM_WRITE_COMMAND);
    // read from memory
    do_basic_trans_check(trans, delay, 0x1000, r_data1, &w_data[8], 16, tlm::tlm_command::TLM_READ_COMMAND);
    print_dashes();
    do_basic_trans_check(trans, delay, 0x2200, r_data2, &w_data[16], 16, tlm::tlm_command::TLM_READ_COMMAND);

    print_dashes();
    signals_writer_event.notify(sc_core::sc_time(sc_core::SC_ZERO_TIME));
    sc_core::wait(sc_core::sc_time(1, sc_core::sc_time_unit::SC_US));
    print_dashes();
    SCP_INFO(()) << "Simulation time = " << sc_core::sc_time_stamp();
    ASSERT_EQ(sc_core::sc_time_stamp(), sc_core::sc_time(4, sc_core::sc_time_unit::SC_US));
}

int sc_main(int argc, char* argv[])
{
    gs::ConfigurableBroker m_broker(
        argc, argv,
        {
            { "test_bench.mem.target_socket.address", cci::cci_value(0x1000) },
            { "test_bench.mem.target_socket.size", cci::cci_value(0x1000) },
            { "test_bench.mem.verbose", cci::cci_value(true) },

            { "test_bench.python-binder.tlm_initiator_ports_num", cci::cci_value(1) },
            { "test_bench.python-binder.tlm_target_ports_num", cci::cci_value(1) },
            { "test_bench.python-binder.initiator_signals_num", cci::cci_value(1) },
            { "test_bench.python-binder.target_signals_num", cci::cci_value(1) },
            { "test_bench.python-binder.target_socket_0.address", cci::cci_value(0x2000) },
            { "test_bench.python-binder.target_socket_0.size", cci::cci_value(0x1000) },
            { "test_bench.python-binder.target_socket_0.relative_addresses", cci::cci_value(false) },
            { "test_bench.python-binder.py_module_dir", cci::cci_value(getexepath()) },
            { "test_bench.python-binder.py_module_name", cci::cci_value("python-binder-test") },
            { "test_bench.python-binder.py_module_args", cci::cci_value("--debug") },
        });

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}