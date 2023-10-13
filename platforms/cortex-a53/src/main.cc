/*
 *  Copyright (C) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <chrono>
#include <string>

#include <cci_configuration>
#include <systemc>

#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/argparser.h>
#include <greensocs/gsutils/module_factory_container.h>

#include <libqbox/components/uart/pl011.h>

class GreenSocsPlatform : public gs::ModuleFactory::Container
{
protected:
    cci::cci_param<int> m_quantum_ns;
    cci::cci_param<int> m_gdb_port;

public:
    GreenSocsPlatform(const sc_core::sc_module_name& n)
        : gs::ModuleFactory::Container(n)
        , m_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
        , m_gdb_port("gdb_port", 0, "GDB port")
    {
        using tlm_utils::tlm_quantumkeeper;

        sc_core::sc_time global_quantum(m_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);
    };
};

int sc_main(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));      // make the msg type column a bit tighter

    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig("sc_main");
    cci::cci_param<int> p_log_level{ "log_level", 0, "Default log level", cci::CCI_ABSOLUTE_NAME, orig };
    auto broker_h = m_broker.create_broker_handle(orig);
    ArgParser ap{ broker_h, argc, argv };

    GreenSocsPlatform platform("platform");

    auto start = std::chrono::system_clock::now();
    try {
        SCP_INFO() << "SC_START";
        sc_core::sc_start();
    } catch (std::runtime_error const& e) {
        std::cerr << argv[0] << "Error: '" << e.what() << std::endl;
        exit(1);
    } catch (const std::exception& exc) {
        std::cerr << argv[0] << " Error: '" << exc.what() << std::endl;
        exit(2);
    } catch (...) {
        SCP_ERR() << "Unknown error (main.cc)!";
        exit(3);
    }

    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds() << "SC_SEC" << std::endl;
    std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)" << std::endl;

    return 0;
}
