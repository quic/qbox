/*
 * Quic Module Cortex-M55
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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
    std::cout << "RemoteCPU started\n";
    std::cout << "Passed arguments: " << std::endl;
    for (int i = 0; i < 2; i++) std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    auto m_broker = new gs::ConfigurableBroker({
        { "remote_platform.moduletype", cci::cci_value("ContainerDeferModulesConstruct") },
        { "remote_platform.quantum_ns", cci::cci_value(10'000'000) },
    });

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