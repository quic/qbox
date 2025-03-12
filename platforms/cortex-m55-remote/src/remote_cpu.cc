/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

/* Quic Module Cortex-M55 */
#include "remote_cpu.h"
#include <module_factory_container.h>

class RemotePlatform : public gs::ModuleFactory::ContainerDeferModulesConstruct
{
    SCP_LOGGER(());

protected:
    cci::cci_param<int> p_quantum_ns;

public:
    RemotePlatform(const sc_core::sc_module_name& n)
        : gs::ModuleFactory::ContainerDeferModulesConstruct(n)
        , p_quantum_ns("quantum_ns", 1'000'000, "TLM-2.0 global quantum in ns")
    {
        using tlm_utils::tlm_quantumkeeper;

        SCP_DEBUG(()) << "Remote Platform Constructor";

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);

        sc_core::sc_module* rpass = construct_module("RemotePass", std::string("plugin_pass").c_str(), {});
        // when we get here, all CCI parameters are communicated from the remote side to the current process after
        // remote_pass is constructed.
        ModulesConstruct();
        name_bind(rpass);
    }

    ~RemotePlatform() {}
};

int sc_main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Error: Expected log_level to be passed as cmdline argument to the remote cpu process!"
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));      // make the msg type column a bit tighter

    gs::ConfigurableBroker m_broker{ {
        { "remote_platform.moduletype", cci::cci_value("ContainerDeferModulesConstruct") },
    } };

    cci::cci_originator orig("sc_main");
    auto broker_h = m_broker.create_broker_handle(orig);
    cci::cci_param<int> p_log_level{ "log_level", std::stoi(argv[1]), "Default log level", cci::CCI_ABSOLUTE_NAME,
                                     orig };

    RemotePlatform remote("remote_platform");
    try {
        sc_core::sc_start();
    } catch (std::runtime_error const& e) {
        std::cerr << "Error: (Remote CPU )  '" << e.what() << "'\n";
        exit(1);
    } catch (const std::exception& err) {
        std::cerr << "Unknown error (Remote CPU) !" << err.what() << "\n";
        exit(2);
    }
    return 0;
}