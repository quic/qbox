/*
 * Copyright (c) 2022 GreenSocs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <functional>
#include <gmock/gmock.h>
#include <memory>
#include <cci/utils/broker.h>
#define GS_USE_TARGET_MODULE_FACTORY
#include <greensocs/base-components/memory.h>
#include <greensocs/base-components/router.h>
#include <greensocs/libgsutils.h>
#include <gtest/gtest.h>
#include <systemc>
#include <tlm>

#include <tlm_utils/simple_initiator_socket.h>

#include <inttypes.h>
#include <systemc>
#include <tlm.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

#include "greensocs/libgsutils.h"
#include "greensocs/gsutils/tests/initiator-tester.h"
#include "greensocs/gsutils/tests/test-bench.h"
#include "greensocs/gsutils/module_factory_container.h"

using testing::AnyOf;
using testing::Eq;

class FactoryPlatform : public gs::ModuleFactory::Container
{

public:
    FactoryPlatform(const sc_core::sc_module_name& n)
        : gs::ModuleFactory::Container(n)
    {
    }
};

int sc_main(int argc, char** argv)
{
    auto m_broker = new gs::ConfigurableBroker(argc, argv);

    // typedef gs::Memory<> Memory;
    // typedef gs::Router<> Router;

    // GSC_MODULE_REGISTER(InitiatorTester);
    //    GSC_MODULE_REGISTER(Memory);
    //    GSC_MODULE_REGISTER(Router);

    FactoryPlatform platform("platform");

    auto mgb = cci::cci_get_global_broker(
        cci::cci_originator("GreenSocs Module Factory test"));
    auto uncon = mgb.get_unconsumed_preset_values();
    for (auto v : uncon) {
        std::cout << "Unconsumed config value: " << v.first << " : " << v.second
                  << "\n";
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(factory_platform, all) { sc_start(1, sc_core::SC_NS); }