/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fss-bench.h"
#include <dlfcn.h>

void FssTestBench::update_time(fssTimeSyncIfHandle time_sync_handle, TimeWindow window)
{
    FssTestBench* self = nullptr;
    if (reinterpret_cast<FssTestBenchTimeSyncIf*>(time_sync_handle)->binderName == "test_fss.ts_binder0") {
        self = gs::container_of<FssTestBench, FssTestBenchTimeSyncIf>(
            reinterpret_cast<FssTestBenchTimeSyncIf*>(time_sync_handle), &FssTestBench::m_time_sync_handle0);
    } else if (reinterpret_cast<FssTestBenchTimeSyncIf*>(time_sync_handle)->binderName == "test_fss.ts_binder1") {
        self = gs::container_of<FssTestBench, FssTestBenchTimeSyncIf>(
            reinterpret_cast<FssTestBenchTimeSyncIf*>(time_sync_handle), &FssTestBench::m_time_sync_handle1);
    } else {
        std::cerr << "FssTestBench::update_time() => Error: time_sync_handle is corrupted!" << std::endl;
    }

    if (self->m_nodes_time_sync_map.at(reinterpret_cast<FssTestBenchTimeSyncIf*>(time_sync_handle)->binderName) ==
        sc_sync_window::zero_window) {
        self->m_nodes_time_sync_map.at(reinterpret_cast<FssTestBenchTimeSyncIf*>(time_sync_handle)->binderName) = {
            sc_core::sc_time::from_seconds(window.from), sc_core::sc_time::from_seconds(window.to)
        };
    }
    if (std::all_of(self->m_nodes_time_sync_map.begin(), self->m_nodes_time_sync_map.end(),
                    [self](const auto entry) { return entry.second == self->updated_time_window; })) {
        std::for_each(self->m_nodes_time_sync_map.begin(), self->m_nodes_time_sync_map.end(),
                      [self](auto& entry) { entry.second = sc_sync_window::zero_window; });
        self->sync_ev.notify(sc_core::SC_ZERO_TIME);
    }
}

void FssTestBench::write_data(gs::fss_data_binder& data_binder, uint64_t address, uint64_t size, uint8_t* data)
{
    bool is_write = true;
    data_binder.set_item(0, &address);
    data_binder.set_item(1, &size);
    data_binder.set_item(2, &is_write);
    data_binder.set_item(3, data);
}
void FssTestBench::read_data(gs::fss_data_binder& data_binder, uint64_t address, uint64_t size, uint8_t* data)
{
    ASSERT_LE(size, /*data size*/ data_binder.get_size(3));
    bool is_write = false;
    data_binder.set_item(0, &address);
    data_binder.set_item(1, &size);
    data_binder.set_item(2, &is_write);
    uint8_t* bus_data = reinterpret_cast<uint8_t*>(data_binder.get_item(3));
    std::copy(bus_data, bus_data + size, data);
}

FssTestBench::FssTestBench(const sc_core::sc_module_name& n)
    : TestBench(n)
    , m_broker(cci::cci_get_broker())
    , p_quantum_ns("quantum_ns", 100, "global quantum in ns")
    , sync_ev("sync_event")
    , cmp0("cmp0")
    , cmp1("cmp1")
{
    SCP_INFO(()) << "FssTestBench constructor";

    sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
    tlm_utils::tlm_quantumkeeper::set_global_quantum(global_quantum);

    // bind events channels of each node
    cmp0.events_binder[0].bind(cmp1.events_binder[0]);
    cmp0.events_binder[1].bind(cmp1.events_binder[1]);

    // bind time sync channel of each node to the FssTestBench time channel
    m_time_sync_handle0.version = (1ULL << 32);
    m_time_sync_handle0.updateTimeWindow = &FssTestBench::update_time;
    m_time_sync_handle0.binderName = "test_fss.ts_binder0";
    m_time_sync_handle1.version = (1ULL << 32);
    m_time_sync_handle1.updateTimeWindow = &FssTestBench::update_time;
    m_time_sync_handle1.binderName = "test_fss.ts_binder1";
    m_sync_window = std::make_shared<sc_core::sc_sync_window<sc_core::sc_sync_policy_tlm_quantum>>("sync_window");
    time_sync_binder0 = std::make_shared<gs::fss_time_sync_binder>(
        "time_sync_binder0", "test_fss", reinterpret_cast<fssTimeSyncIfHandle>(&m_time_sync_handle0));
    time_sync_binder1 = std::make_shared<gs::fss_time_sync_binder>(
        "time_sync_binder1", "test_fss", reinterpret_cast<fssTimeSyncIfHandle>(&m_time_sync_handle1));
    cmp0.time_sync_binder[0].bind(*time_sync_binder0);
    cmp1.time_sync_binder[0].bind(*time_sync_binder1);

    m_nodes_time_sync_map.insert(std::make_pair("test_fss.ts_binder0", sc_sync_window::zero_window));
    m_nodes_time_sync_map.insert(std::make_pair("test_fss.ts_binder1", sc_sync_window::zero_window));
    m_sync_window->register_sync_cb([this](const window_t& w) {
        updated_time_window = w;
        SCP_DEBUG(()) << "send time window: [" << w.from << " - " << w.to << "] to Node: " << cmp0.name();
        time_sync_binder0->update_time({ w.from.to_seconds(), w.to.to_seconds() });
        SCP_DEBUG(()) << "send time window: [" << w.from << " - " << w.to << "] to Node: " << cmp1.name();
        time_sync_binder1->update_time({ w.from.to_seconds(), w.to.to_seconds() });
    });

    // bind data cahnnel of each node
    cmp0.data_binder[0].bind(cmp1.data_binder[0]);

    // bind control channel of each node
    cmp0.control_binder[0].bind(cmp1.control_binder[0]);

    SC_METHOD(sync);
    dont_initialize();
    sensitive << sync_ev;
}

void FssTestBench::sync()
{
    auto from_t = updated_time_window.from;
    auto to_t = sc_core::sc_time_stamp() + sc_core::sc_time(p_quantum_ns.get_value(), sc_core::sc_time_unit::SC_NS);
    SCP_DEBUG(()) << "update time window to: [" << from_t << " - " << to_t << "]";
    m_sync_window->async_set_window({ from_t, to_t });
}

TEST_BENCH(FssTestBench, test_fss)
{
    sc_core::wait(300, sc_core::sc_time_unit::SC_NS);
    uint64_t data = 0x02020202;
    write_data(cmp0.data_binder[0], 4, 4, reinterpret_cast<uint8_t*>(&data));
    cmp0.data_binder[0].transmit();
    data = 0;
    read_data(cmp0.data_binder[0], 4, 4, reinterpret_cast<uint8_t*>(&data));
    cmp0.data_binder[0].transmit();
    ASSERT_EQ(data, 0x02020202);

    rapidjson::Document doc_get, doc_set;
    doc_get.SetObject();
    doc_set.SetObject();
    doc_get.AddMember("command", rapidjson::Value("get_reg"), doc_get.GetAllocator());
    doc_get.AddMember("address", rapidjson::Value(0x4), doc_get.GetAllocator());
    rapidjson::StringBuffer str_get, str_set;
    rapidjson::Writer<rapidjson::StringBuffer> writer_get(str_get);
    doc_get.Accept(writer_get);
    auto cmd_result = cmp0.control_binder[0].do_command(str_get.GetString());
    ASSERT_NE(cmd_result, nullptr);
    std::cout << "Json: " << str_get.GetString() << " : " << cmd_result << std::endl;

    doc_set.Parse(str_get.GetString());
    doc_set["command"].SetString(std::string("set_reg").c_str(), doc_set.GetAllocator());
    doc_set.AddMember("value", rapidjson::Value(0x13131313), doc_set.GetAllocator());
    rapidjson::Writer<rapidjson::StringBuffer> writer_set(str_set);
    doc_set.Accept(writer_set);
    ASSERT_NE(cmp0.control_binder[0].do_command(str_set.GetString()), nullptr);
    data = 0;
    read_data(cmp0.data_binder[0], 4, 4, reinterpret_cast<uint8_t*>(&data));
    ASSERT_EQ(data, 0x13131313);

    sc_core::wait(600, sc_core::sc_time_unit::SC_NS);
    sc_core::sc_stop();
}

void FssTestBench::before_end_of_elaboration()
{
    cmp0.events_binder[0].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp1.events_binder[0].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp0.events_binder[1].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp1.events_binder[1].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
}

void FssTestBench::end_of_elaboration()
{
    cmp0.events_binder[0].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp1.events_binder[0].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp0.events_binder[1].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp1.events_binder[1].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
}

void FssTestBench::start_of_simulation()
{
    cmp0.events_binder[0].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp1.events_binder[0].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp0.events_binder[1].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
    cmp1.events_binder[1].notify(static_cast<uint64_t>(sc_core::sc_get_status()));
}

int sc_main(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(50));      // make the msg type column a bit tighter
    std::string lib_name = getexepath() + "/fss_node";
#ifdef _WIN32
    lib_name += ".dll";
#elif __APPLE__
    lib_name += ".dylib";
#else
    lib_name += ".so";
#endif
    gs::ConfigurableBroker m_broker({ { "test_fss.cmp0.node_exe_path", cci::cci_value(lib_name) },
                                      { "test_fss.cmp0.node_name", cci::cci_value("fss_node0") },
                                      { "test_fss.cmp0.node_configs.baud_rate", cci::cci_value(9200) },
                                      { "test_fss.cmp0.events_binder_0.name", cci::cci_value("test_fss.cmp0") },
                                      { "test_fss.cmp0.events_binder_1.name", cci::cci_value("test_fss.cmp0.aux") },
                                      { "test_fss.cmp0.time_sync_binder_0.name", cci::cci_value("test_fss.cmp0") },
                                      { "test_fss.cmp0.data_binder_0.name", cci::cci_value("test_fss.cmp0") },
                                      { "test_fss.cmp0.control_binder_0.name", cci::cci_value("test_fss.cmp0") },
                                      { "test_fss.cmp1.node_exe_path", cci::cci_value(lib_name) },
                                      { "test_fss.cmp1.node_name", cci::cci_value("fss_node1") },
                                      { "test_fss.cmp1.node_configs.baud_rate", cci::cci_value(9200) },
                                      { "test_fss.cmp1.events_binder_0.name", cci::cci_value("test_fss.cmp1") },
                                      { "test_fss.cmp1.events_binder_1.name", cci::cci_value("test_fss.cmp1.aux") },
                                      { "test_fss.cmp1.time_sync_binder_0.name", cci::cci_value("test_fss.cmp1") },
                                      { "test_fss.cmp1.data_binder_0.name", cci::cci_value("test_fss.cmp1") },
                                      { "test_fss.cmp1.control_binder_0.name", cci::cci_value("test_fss.cmp1") },
                                      { "test_fss.quantum_ns", cci::cci_value(100) } });

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}