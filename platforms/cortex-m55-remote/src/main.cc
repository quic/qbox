/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

/* M55 mini-vp */
#include <chrono>
#include <string>

#include <cci_configuration>

#include <cciutils.h>
#include <argparser.h>
#include "remote_cpu.h"

#include <keep_alive/include/keep_alive.h>
#include <module_factory_container.h>

#if SC_VERSION_MAJOR < 3
#warning PLEASE UPDATE TO SYSTEMC 3.0, OLDER VERSIONS ARE DEPRECATED AND MAY NOT WORK
#endif

class GreenSocsPlatform : public gs::ModuleFactory::Container
{
protected:
    cci::cci_param<int> p_quantum_ns;

public:
    GreenSocsPlatform(const sc_core::sc_module_name& n)
        : gs::ModuleFactory::Container(n), p_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
    {
        using tlm_utils::tlm_quantumkeeper;

        SCP_DEBUG(()) << "Constructor";

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);
    }

    SCP_LOGGER(());

    ~GreenSocsPlatform() {}
};

int sc_main(int argc, char* argv[])
{
    if (sc_core::sc_version_major < 3) {
        SCP_WARN()
        ("\n********************************\nWARNING, USING A DEPRECATED VERSION OF SYSTEMC, PLEASE "
         "UPGRADE\n********************************");
    }

    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig{ "sc_main" };
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
        SCP_ERR() << "Local platform Unknown error (main.cc)!";
        exit(3);
    }
    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds() << "SC_SEC" << std::endl;
    std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)" << std::endl;

    return 0;
}