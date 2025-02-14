/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <gmock/gmock.h>
#include <libgsutils.h>
#include <gtest/gtest.h>
#include <scp/helpers.h>
#include <scp/report.h>
#include <scp/tlm_extensions/initiator_id.h>
#include <scp/tlm_extensions/path_trace.h>
#include <systemc>

using testing::AnyOf;
using testing::Eq;

SC_MODULE (testA) {
    void testA_method()
    {
        SCP_WARN(SCMOD) << "SCP_WARN(SCMOD)";     // displayed if SCMOD (testA.log_level>= SC_LOW which
                                                  // means CCI_value>=1, Verbosity >= 100) or
                                                  // log_level>=1 if SCMOD.log_level not defined
        SCP_WARN() << "SCP_WARN()";               // always printed unless we set
                                                  // scp::LogConfig().logLevel(scp::log::NONE)
        SCP_WARN("bar.me") << "SCP_WARN(bar.me)"; // displayed if the value of bar.me.log_level >=1 or log_level>=1

        SCP_INFO(SCMOD) << "SCP_INFO(SCMOD)"; // displayed if SCMOD (testA.log_level>= SC_MEDIUM
                                              // which means CCI_value>=4, Verbosity >= 200) or or
                                              // log_level>=4 if SCMOD.log_level not defined
        SCP_INFO() << "SCP_INFO()";           // always printed unless we set
                                              // scp::LogConfig().logLevel(scp::log::WARNING) or LOWER value
                                              // (NONE, FATAL,ERORR, WARNING)
        SCP_INFO("") << "SCP_INFO(empty)";    //  displayed if log_level>=4
        SCP_INFO("foo.me.toto") << "SCP_INFO(foo.me.toto)"; // displayed if foo.me.toto.log_level>=4 or
                                                            // foo.me.log_level>=4 or foo.log_level>=4 or log_level>=4
        SCP_INFO("bar.me") << "SCP_INFO(bar.me)";           // displayed if  bar.me.log_level>=4 or
                                                            // bar.log_level>=4 or log_level>=4
        SCP_INFO("bar") << "SCP_INFO(bar)";                 // displayed if bar.log_level>=4  or log_level>=4

        SCP_DEBUG(SCMOD) << "SCP_DEBUG(SCMOD)"; // displayed if SCMOD (testA.log_level>= SC_HIGH
                                                // which means CCI_value>=5, Verbosity >= 300) or or
                                                // log_level>=5 if SCMOD.log_level not defined
        SCP_DEBUG() << "SCP_DEBUG()";           // always printed unless we set
                                                // scp::LogConfig().logLevel(scp::log::INFO) or LOWER value
                                                // (NONE, FATAL,ERORR, WARNING, INFO)
        SCP_DEBUG("") << "SCP_DEBUG(empty)";    //  displayed if log_level>= 5
        SCP_DEBUG("DEBUG.foo.me") << "SCP_DEBUG(DEBUG.foo.me)"; // displayed if DEBUG.foo.me.log_level>=5 or
                                                                // DEBUG.foo.log_level>=5 or DEBUG.log_level>=4 or
                                                                // log_level>=5
        SCP_DEBUG("DEBUG.bar") << "SCP_DEBUG(DEBUG.bar)";       // displayed if  DEBUG.bar.log_level>=5 or
                                                                // DEBUG.log_level>=5 or log_level>=5
    }

    SC_CTOR (testA) {
        SC_METHOD(testA_method);
    }
};

SC_MODULE (testB) {
    void testB_method()
    {
        int data;
        tlm::tlm_command cmd = static_cast<tlm::tlm_command>(tlm::TLM_READ_COMMAND);
        tlm::tlm_generic_payload txn;

        txn.set_command(cmd);
        txn.set_address(0x00);
        txn.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        txn.set_data_length(4);
        txn.set_streaming_width(4);
        txn.set_byte_enable_ptr(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        scp::tlm_extensions::path_trace ext;
        ext.stamp(this);
        SCP_INFO(SCMOD) << ext.to_string();
        ext.reset();

        ext.stamp(this);
        ext.stamp(this);
        ext.stamp(this);

        SCP_INFO(SCMOD) << ext.to_string();
        ext.reset();

        txn.set_extension(&ext);
        scp::tlm_extensions::initiator_id mid(0x1234);
        mid = 0x2345;
        mid &= 0xff;
        mid <<= 4;
        uint64_t myint = mid + mid;
        myint += mid;

        if (mid == 0x450) {
            SC_REPORT_INFO("ext test", "Success");
            SCP_INFO(SCMOD) << scp::scp_txn_tostring(
                txn); // scp_txn_tostring print the txn information and its extensions - displayed
                      // if testB.log_level>=4  or log_level>=4 if testB.log_level not defined
        } else {
            SC_REPORT_INFO("ext test", "Failour");
        }
        txn.clear_extension(&ext);
    }

    SC_CTOR (testB) {
        SC_METHOD(testB_method);
    }
};

int sc_main(int argc, char** argv)
{
    scp::init_logging(scp::LogConfig()
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(10));      // make the msg type column a bit tighter

    auto m_broker = new gs::ConfigurableBroker();

    testA t1("testA");
    testB t2("testB");

    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}

TEST(luatest, all) { sc_start(1, sc_core::SC_NS); }
