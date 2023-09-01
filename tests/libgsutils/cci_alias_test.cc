/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
class One : public sc_core::sc_module
{
private:
public:
    cci::cci_param<int, cci::CCI_IMMUTABLE_PARAM> m_thing;
    One(const sc_core::sc_module_name& n, int _thing = 0): m_thing("thing", _thing)
    {
        set_value += m_thing;
        SCP_INFO(SCMOD) << name() << " thing " << m_thing;
    }

    void end_of_elaboration() { SCP_INFO(SCMOD) << name() << " EOE : thing " << m_thing; }
};

class Muty : public sc_core::sc_module
{
private:
public:
    cci::cci_param<int> m_thing_muty;
    Muty(const sc_core::sc_module_name& n, int _thing = 0): m_thing_muty("thing_muty", _thing)
    {
        SCP_INFO(SCMOD) << name() << " thing " << m_thing_muty;
    }

    void end_of_elaboration()
    {
        // This should be the value set at construction, not the
        // value at EOE because it's marked as IMMUTABLE
        EXPECT_EQ(m_thing_muty, 20);
        std::cout << name() << " EOE : thing " << m_thing_muty << std::endl;
    }
};

/*
 * Well behaved 'top level' that keeps it's param's private
 */
class TopLevel : public sc_core::sc_module
{
protected:
    gs::ConfigurableBroker m_priv_broker;
    cci::cci_param<int> my_thing;
    One one, two, three;
    Muty m1;

public:
    TopLevel(const sc_core::sc_module_name& n)
        : m_priv_broker(
              { { "one.thing", cci::cci_value(10) } },
              { { "two.thing", "my_thing" }, { "three.thing", "my_thing" }, { "muty1.thing_muty", "my_thing" } })
        , my_thing("my_thing", 20)
        , one("one")
        , two("two")
        , three("three")
        , m1("muty1")
    {
    }

    void end_of_elaboration()
    {
        my_thing = 60;
        SCP_INFO(SCMOD) << name() << " EOE : thing " << my_thing;
    }
};

int sc_main(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(10));      // make the msg type column a bit tighter

    auto m_broker = new gs::ConfigurableBroker(argc, argv);
    cci::cci_originator m_originator("MyConfigTool");
    m_broker->set_preset_cci_value("top.one.thing", cci::cci_value(cci::cci_value::from_json("30")), m_originator);
    m_broker->set_preset_cci_value("top.two.thing", cci::cci_value(cci::cci_value::from_json("40")), m_originator);

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

TEST(cci_alias_test, one)
{
    TopLevel tl("top");
    sc_start(1, sc_core::SC_NS);

    EXPECT_EQ(set_value, 50);
}
