/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <functional>
#include <gmock/gmock.h>
#include <memory>
#include <cci/utils/broker.h>

#include <gs_memory.h>
#include <router.h>
#include <libgsutils.h>
#include <gtest/gtest.h>
#include <systemc>
#include <tlm>

#include <tlm_utils/simple_initiator_socket.h>

#include <inttypes.h>
#include <systemc>
#include <tlm.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

#include <argparser.h>
#include "tests/initiator-tester.h"
#include "tests/test-bench.h"
#include "module_factory_container.h"

using testing::AnyOf;
using testing::Eq;

class FactoryPlatform : public gs::ModuleFactory::Container
{
public:
    FactoryPlatform(const sc_core::sc_module_name& n): gs::ModuleFactory::Container(n) {}
};

int sc_main(int argc, char** argv)
{
    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig{ "sc_main" };
    auto broker_h = m_broker.create_broker_handle(orig);
    ArgParser ap{ broker_h, argc, argv };

    FactoryPlatform platform("platform");

    auto mgb = cci::cci_get_global_broker(cci::cci_originator("GreenSocs Module Factory test"));
    auto uncon = mgb.get_unconsumed_preset_values();
    for (auto v : uncon) {
        std::cout << "Unconsumed config value: " << v.first << " : " << v.second << "\n";
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(factory_platform, all) { sc_start(1, sc_core::SC_NS); }