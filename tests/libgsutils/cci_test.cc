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

#include <cci_configuration>
#include <gmock/gmock.h>
#include <greensocs/libgsutils.h>
#include <gtest/gtest.h>
#include <systemc>
#include <scp/report.h>
// global for test
int set_value = 0;
/*
 * Module with a structural param
 */
class IrqCtrl : public sc_core::sc_module
{
private:
public:
    cci::cci_param<int, cci::CCI_IMMUTABLE_PARAM> m_irqs;
    IrqCtrl(const sc_core::sc_module_name& n, int n_irq = 0): m_irqs("irqs", n_irq) {
        set_value = m_irqs;
        SCP_INFO(SCMOD) << "Number of IRQs " << m_irqs;
        if (::scp::get_log_verbosity(SCMOD) >= sc_core::SC_LOW) {
            EXPECT_NE(m_irqs, 99); // the 3rd (99) should be disable on the command line
            SCP_INFO(SCMOD) << "Report test output";
        } else {
            EXPECT_EQ(m_irqs, 99);
        }
    }
};

/*
 * Well behaved 'top level' that keeps it's param's private
 */
class TopLevel : public sc_core::sc_module
{
protected:
    gs::ConfigurableBroker m_priv_broker;
    IrqCtrl m_ctrl;

public:
    TopLevel(const sc_core::sc_module_name& n)
        : m_priv_broker({ { "MyCtrl.irqs", cci::cci_value() } }), m_ctrl("MyCtrl", 64) {}
    // Add irqs to the list so it is not exported, but take it's value from the passed value
};

/*
 * badly behaved module, that sets the structural param but does not
 * capture it in a private ebroker
 */
class TopLevelTwo : public sc_core::sc_module
{
protected:
    gs::ConfigurableBroker m_priv_broker;
    IrqCtrl m_ctrl;

public:
    TopLevelTwo(const sc_core::sc_module_name& n): m_priv_broker({ {} }), m_ctrl("MyCtrl", 64) {}
};

/*
 * reasonably behaved module, that uses 'standard' configuration
 * NB this is broken on the released version of CCI
 */
class TopLevelThree : public sc_core::sc_module
{
protected:
    gs::ConfigurableBroker m_priv_broker;
    IrqCtrl m_ctrl;

public:
    TopLevelThree(const sc_core::sc_module_name& n)
        : m_priv_broker({ { "MyCtrl.irqs", cci::cci_value(42) } }), m_ctrl("MyCtrl") {}
};
TopLevel* top_level1;
TopLevelTwo* top_level2;
TopLevelThree* top_level3;
int sc_main(int argc, char* argv[]) {
    std::cout << "Test Start\nstd::cout print\n";
    SCP_INFO("main") << "SCP_INFO(\"main\") prior to init_logging";
    scp::init_logging(scp::LogConfig()
                          .logAsync(false)
                          .printSimTime(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(50));      // make the msg type column a bit tighter
    SCP_INFO("main") << "SCP_INFO(\"main\") between init_logging and broker construction (Causes "
                        "ERROR which is caught)";
    auto m_broker = new gs::ConfigurableBroker(argc, argv);
    SCP_INFO("main") << "SCP_INFO(\"main\") after broker construction";

    cci::cci_originator m_originator("MyConfigTool");
    m_broker->set_preset_cci_value("MyTop.MyCtrl.irqs",
                                   cci::cci_value(cci::cci_value::from_json("100")), m_originator);
    m_broker->set_preset_cci_value("MyTopTwo.MyCtrl.irqs",
                                   cci::cci_value(cci::cci_value::from_json("200")), m_originator);
    m_broker->set_preset_cci_value("MyTopThree.MyCtrl.irqs",
                                   cci::cci_value(cci::cci_value::from_json("300")), m_originator);

    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();

    sc_start(1, sc_core::SC_NS);

    auto params = m_broker->get_param_handles(m_originator);
    for (auto v : params) {
        SCP_INFO("sc_main") << "config value: " << v.name() << " : " << v.get_cci_value();
    }

    auto uncon = m_broker->get_unconsumed_preset_values();
    for (auto v : uncon) {
        SCP_INFO("sc_main") << "Unconsumed config value: " << v.first << " : " << v.second;
    }

    return status;
}

TEST(ccitest, one) {
    top_level1 = new TopLevel("MyTop");
    EXPECT_EQ(set_value, 64);
}
TEST(ccitest, two) {
    top_level2 = new TopLevelTwo("MyTopTwo");
    EXPECT_EQ(set_value, 200);
}
TEST(ccitest, three) {
    top_level3 = new TopLevelThree("MyTopThree");
    EXPECT_EQ(set_value, 99);
}
