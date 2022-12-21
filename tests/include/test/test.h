/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs SAS
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include <systemc>
#include <cci/utils/broker.h>

#include <greensocs/gsutils/cciutils.h>

#include <scp/report.h>

class TestFailureException : public std::runtime_error
{
protected:
    std::string* m_what;

    const char* make_what(const char* what, const char* func, const char* file, int line) {
        std::stringstream ss;
        ss << what << " (in " << func << ", at " << file << ":" << line << ")";

        m_what = new std::string(ss.str());

        return m_what->c_str();
    }

public:
    TestFailureException(const char* what, const char* func, const char* file, int line)
        : std::runtime_error(make_what(what, func, file, line)) {}
    virtual ~TestFailureException() noexcept { delete m_what; }
};

class TestBench : public sc_core::sc_module
{
private:
    bool m_test_success = true;

public:
    TestBench(const sc_core::sc_module_name& n): sc_core::sc_module(n) {}

    virtual ~TestBench() {}

    void run() {
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
int run_testbench(int argc, char* argv[]) {
    auto m_broker = new gs::ConfigurableBroker(argc, argv);
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));      // make the msg type column a bit tighter

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
