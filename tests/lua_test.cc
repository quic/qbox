#include <gmock/gmock.h>
#include <greensocs/libgsutils.h>
#include <gtest/gtest.h>
#include <systemc>

using testing::AnyOf;
using testing::Eq;

SC_MODULE(testA) {
  cci::cci_param<int> defvalue;
  cci::cci_param<int> cmdvalue;
  cci::cci_param<int> luavalue;
  cci::cci_param<int> allvalue;
  void testA_method() {
    std::cout << "test def value = " << defvalue << std::endl;
    std::cout << "test cmd value = " << cmdvalue << std::endl;
    std::cout << "test lua value = " << luavalue << std::endl;
    std::cout << "test all value = " << allvalue << std::endl;

    EXPECT_EQ(defvalue, 1234);
    EXPECT_EQ(cmdvalue, 1010);
    EXPECT_EQ(luavalue, 2020);
    EXPECT_EQ(allvalue, 1050); // last one to write wins, in this case the command line
  }
  SC_CTOR(testA)
      : defvalue("defvalue", 1234)
      , cmdvalue("cmdvalue", 1234)
      , luavalue("luavalue", 1234)
      , allvalue("allvalue", 1234) {
    SC_METHOD(testA_method);
  }
};

int sc_main(int argc, char **argv) {
  auto m_broker = new cci_utils::broker("Global Broker");
  cci::cci_register_broker(m_broker);
  LuaFile_Tool lua("lua");
  lua.parseCommandLine(argc, argv);

  testA t1("top");
  
  testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  return status;

}

TEST(luatest , all) {
  sc_start(1, sc_core::SC_NS);
}
