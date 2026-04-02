/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include "gs_memory.h"
#include "router.h"
#include <tests/initiator-tester.h>
#include <tests/test-bench.h>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <filesystem>
#include <rapidjson/include/rapidjson/document.h>
#include <rapidjson/include/rapidjson/writer.h>
#include <rapidjson/include/rapidjson/stringbuffer.h>
#include <component_constructor.h>
#include <sync_window.h>
#include <fss_interfaces.h>
#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#include <errno.h>
#endif

std::string getexepath()
{
    char result[1024] = { 0 };
#ifdef _WIN32
    if (!GetModuleFileNameW(NULL, result, sizeof(result))) {
        std::cerr << "getexepath() -> GetModuleFileNameW() Failed! Error: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
#elif __APPLE__
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) != 0) {
        std::cerr << "getexepath() -> _NSGetExecutablePath() Failed! Check buffer size!" << std::endl;
        exit(EXIT_FAILURE);
    }
#else
    if (readlink("/proc/self/exe", result, sizeof(result)) < 0) {
        std::cerr << "getexepath() -> readlink() Failed! Error: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
    return std::filesystem::path(result).parent_path();
}

typedef struct FssTestBenchTimeSyncIf {
    fssUint64 version;
    fssUpdateTimeWindowFnPtr updateTimeWindow;
    fssString binderName;
} FssTestBenchTimeSyncIf;

class FssTestBench : public TestBench
{
    SCP_LOGGER(());
    using sc_sync_window = typename sc_core::sc_sync_window<sc_core::sc_sync_policy_tlm_quantum>;
    using window_t = sc_sync_window::window;

public:
    FssTestBench(const sc_core::sc_module_name& n);
    static void update_time(fssTimeSyncIfHandle time_sync_handle, TimeWindow window);
    void write_data(gs::fss_data_binder& data_binder, uint64_t address, uint64_t size, uint8_t* data);
    void read_data(gs::fss_data_binder& data_binder, uint64_t address, uint64_t size, uint8_t* data);
    void sync();
    virtual ~FssTestBench() {}

private:
    void before_end_of_elaboration();
    void end_of_elaboration();
    void start_of_simulation();

protected:
    cci::cci_broker_handle m_broker;
    cci::cci_param<int> p_quantum_ns;
    sc_core::sc_event sync_ev;
    FssTestBenchTimeSyncIf m_time_sync_handle0;
    FssTestBenchTimeSyncIf m_time_sync_handle1;
    std::shared_ptr<gs::fss_time_sync_binder> time_sync_binder0;
    std::shared_ptr<gs::fss_time_sync_binder> time_sync_binder1;
    std::unordered_map<fssString, window_t> m_nodes_time_sync_map;
    std::shared_ptr<sc_core::sc_sync_window<sc_core::sc_sync_policy_tlm_quantum>> m_sync_window;
    window_t updated_time_window;
    gs::component_constructor cmp0;
    gs::component_constructor cmp1;
};