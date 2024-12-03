/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <gmock/gmock.h>
#include <libgsutils.h>
#include <argparser.h>
#include <gtest/gtest.h>
#include <systemc>
#include <scp/report.h>
using testing::AnyOf;
using testing::Eq;

SC_MODULE (testA) {
    cci::cci_param<int> defvalue;
    cci::cci_param<int> cmdvalue;
    cci::cci_param<int> luavalue;
    cci::cci_param<int> allvalue;
    void testA_method()
    {
        SCP_INFO(SCMOD) << "test def value = " << defvalue;
        SCP_INFO(SCMOD) << "test cmd value = " << cmdvalue;
        SCP_INFO(SCMOD) << "test lua value = " << luavalue;
        SCP_INFO(SCMOD) << "test all value = " << allvalue;

        EXPECT_EQ(defvalue, 1234);
        EXPECT_EQ(cmdvalue, 1010);
        EXPECT_EQ(luavalue, 2020);
        EXPECT_EQ(allvalue, 1050); // last one to write wins, in this case the command line
    }
    SC_CTOR (testA)
      : defvalue("defvalue", 1234)
      , cmdvalue("cmdvalue", 1234)
      , luavalue("luavalue", 1234)
      , allvalue("allvalue", 1234)
      {
          SC_METHOD(testA_method);
      }
};

int sc_main(int argc, char** argv)
{
    scp::init_logging(scp::LogConfig()
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(10));      // make the msg type column a bit tighter

    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig{ "sc_main" };
    auto broker_h = m_broker.create_broker_handle(orig);
    ArgParser ap{ broker_h, argc, argv };

    testA t1("top");

    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}

TEST(luatest, all) { sc_start(1, sc_core::SC_NS); }
