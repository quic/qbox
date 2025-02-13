// Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TEST_H
#define TEST_H

#include <iostream>
#include <sstream>
#include <string>

#include <systemc>

#include "argparser.h"
#include "cciutils.h"
#include "cci/utils/broker.h"
#include "gs_memory.h"
#include "keystone/keystone.h"
#include "scp/report.h"

bool run_systemc()
{
    bool test_failed = true;

    try {
        sc_core::sc_start();
        if (sc_core::SC_STOPPED != sc_core::sc_get_status()) {
            sc_core::sc_stop();
        }

        test_failed = false;
    } catch (std::exception& e) {
        std::cerr << "Test failure: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "Test failure: Unknown exception\n";
    }

    return test_failed;
}

void load_firmware(gs::gs_memory<>& destination, const char* assembly, uint64_t addr = 0)
{
    ks_engine* ks = nullptr;

    ks_err err = ks_open(KS_ARCH_HEXAGON, KS_MODE_LITTLE_ENDIAN, &ks);
    if (KS_ERR_OK != err) {
        SCP_FATAL() << "Unable to initialize keystone";
    }

    size_t count = 0;
    uint8_t* fw = nullptr;
    size_t size = 0;

    if (ks_asm(ks, assembly, addr, &fw, &size, &count)) {
        SCP_FATAL() << "Unable to assemble the test firmware: " << ks_strerror(ks_errno(ks)) << " (" << ks_errno(ks)
                    << ")";
    }

    destination.load.ptr_load(fw, addr, size);

    ks_free(fw);
    ks_close(ks);
}

template <typename T>
bool run_test(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::INFO)
                          .msgTypeFieldWidth(30));

    gs::ConfigurableBroker broker{};
    cci::cci_originator originator{ "test" };

    auto broker_handle = broker.create_broker_handle(originator);
    ArgParser arg_parser{ broker_handle, argc, argv };

    T test{ "test" };

    return run_systemc();
}

#define RUN_TEST(test) \
    int sc_main(int argc, char* argv[]) { return run_test<test>(argc, argv); }

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

#endif
