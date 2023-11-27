/*
 *  M55 mini-vp
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <chrono>
#include <string>

#include <cci_configuration>
#include <systemc>

#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/luafile_tool.h>
#include "remote_cpu.h"

#include <greensocs/base-components/memory.h>
#include <greensocs/base-components/router.h>
#include <greensocs/base-components/remote.h>
#include "greensocs/systemc-uarts/uart-pl011.h"
#include "greensocs/systemc-uarts/backends/char-backend.h"
#include <greensocs/systemc-uarts/backends/char/stdio.h>
#include <greensocs/libgssync/keep_alive.h>
#include <greensocs/gsutils/module_factory_container.h>

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
    auto m_broker = new gs::ConfigurableBroker(argc, argv);

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