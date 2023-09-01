/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gmock/gmock.h>
#include <unistd.h>
#include <greensocs/libgsutils.h>
#include <gtest/gtest.h>
#include <scp/helpers.h>
#include <scp/report.h>
#include <systemc>

#include "libgssync/async_event.h"

class TestA : public sc_core::sc_module
{
private:
    gs::async_event m_event;

public:
    TestA(const sc_core::sc_module_name& n): sc_core::sc_module(n)
    {
        SCP_INFO(SCMOD) << "In the constructor of TestA";

        SC_METHOD(testA_method);
        sensitive << m_event;
        dont_initialize();
    }

    ~TestA()
    {
        m_thread2->join();
        m_thread->join();
    }

private:
    std::unique_ptr<std::thread> m_thread;
    std::unique_ptr<std::thread> m_thread2;

    void testA_method()
    {
        SCP_INFO(SCMOD) << "testA method";
        sc_core::sc_stop();
    }

    void start_of_simulation() { m_thread = std::make_unique<std::thread>(&TestA::funct_thread_1, this); }

    void funct_thread_1()
    {
        SCP_INFO(SCMOD) << "Print in the thread1";
        m_thread2 = std::make_unique<std::thread>(&TestA::print_with_scp_report, this);
    }

    void print_with_scp_report()
    {
        SCP_INFO(SCMOD) << "Print in the thread2";
        m_event.notify(sc_core::sc_time(10, sc_core::SC_NS));
    }
};

class TestB : public sc_core::sc_module
{
public:
    void testB_method() { SCP_INFO(SCMOD) << "testB method"; }

    TestB(const sc_core::sc_module_name& n): sc_core::sc_module(n)
    {
        SCP_INFO(SCMOD) << "In the constructor of TestB";
        SC_METHOD(testB_method);
    }
};

int sc_main(int argc, char** argv)
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(30));      // make the msg type column a bit tighter

    auto m_broker = new gs::ConfigurableBroker(argc, argv);
    SCP_INFO("sc_main") << "Inside the sc_main";
    TestA t1("testA");
    TestB t2("testB");

    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}

TEST(thread_test, all) { sc_core::sc_start(); }