/*
 * This file is part of libqbox
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include <systemc>
#include <cci/utils/broker.h>

#include <cciutils.h>
#include <argparser.h>

#include <scp/report.h>

class TestFailureException : public std::runtime_error
{
protected:
    std::string* m_what;

    const char* make_what(const char* what, const char* func, const char* file, int line)
    {
        std::stringstream ss;
        ss << what << " (in " << func << ", at " << file << ":" << line << ")";

        m_what = new std::string(ss.str());

        return m_what->c_str();
    }

public:
    TestFailureException(const char* what, const char* func, const char* file, int line)
        : std::runtime_error(make_what(what, func, file, line))
    {
    }
    virtual ~TestFailureException() noexcept { delete m_what; }
};

class TestBench : public sc_core::sc_module
{
private:
    bool m_test_success = true;

public:
    TestBench(const sc_core::sc_module_name& n): sc_core::sc_module(n) { SCP_INFO(SCMOD) << "Test Bench Constructor"; }

    virtual ~TestBench() {}

    void run()
    {
        try {
            sc_core::sc_start();

            if (sc_core::sc_get_status() != sc_core::SC_STOPPED) {
                sc_core::sc_stop(); /* For end_of_simulation callbacks */
            }
        } catch (std::exception& e) {
            std::cerr << "Test failure: " << e.what() << "\n";
            m_test_success = false;
        }
    }

    int get_rc() const { return !m_test_success; }
};

template <class TESTBENCH>
int run_testbench(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::INFO) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));  // make the msg type column a bit tighter
    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig{ "sc_main" };
    auto broker_h = m_broker.create_broker_handle(orig);
    ArgParser ap{ broker_h, argc, argv };

    SCP_INFO("test.h") << "Start Test";
    TESTBENCH test_bench("test-bench");

    test_bench.run();
    SCP_INFO("test.h") << "Test done";
    return test_bench.get_rc();
}

#define TEST_FAIL(what)                                                 \
    do {                                                                \
        throw TestFailureException(what, __func__, __FILE__, __LINE__); \
    } while (0)

#define TEST_ASSERT(assertion)                              \
    do {                                                    \
        if (!(assertion)) {                                 \
            TEST_FAIL("assertion `" #assertion "' failed"); \
        }                                                   \
    } while (0)
