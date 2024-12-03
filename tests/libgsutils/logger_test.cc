/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include <systemc>
#include <libgsutils.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <scp/report.h>

using testing::AnyOf;
using testing::Eq;

SC_MODULE (testA) {
    void testA_method()
    {
        std::cout << "(cout) test A" << std::endl;
        SC_REPORT_INFO("testA", "(SC_REPORT_INFO) test A");
        SCP_INFO() << "(SCP_INFO) test A";
    }
    SC_CTOR (testA) {
        SC_METHOD(testA_method);
    }
};

int sc_main(int argc, char** argv)
{
    scp::init_logging(scp::LogConfig()
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(10));      // make the msg type column a bit tighter

    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    std::cout << "(cout) Logger test" << std::endl;
    SC_REPORT_INFO("sc_main", "(SC_REPORT_INFO) Logging test");

    testA atest("TestA_Instance");

    sc_core::sc_start();

    std::cout.rdbuf(old);

    std::string text = buffer.str(); // text will now contain "Bla\n"
    std::cout << "Text found: " << std::endl << text << std::endl;

    return EXIT_SUCCESS;
}