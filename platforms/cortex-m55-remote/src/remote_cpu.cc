/*
 * Quic Module Cortex-M55
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "remote_cpu.h"
#include <greensocs/gsutils/module_factory_container.h>

class RemotePlatform : public gs::ModuleFactory::ContainerDeferModulesConstruct
{
    SCP_LOGGER(());

protected:
    cci::cci_param<int> p_quantum_ns;

public:
    RemotePlatform(const sc_core::sc_module_name& n)
        : gs::ModuleFactory::ContainerDeferModulesConstruct(n)
        , p_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
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
    auto m_broker = new gs::ConfigurableBroker(
        argc, argv,
        {
            { "remote_platform.moduletype", cci::cci_value("ContainerDeferModulesConstruct") },
            { "remote_platform.quantum_ns", cci::cci_value(10000000) },
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