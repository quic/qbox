#include <systemc>
#include <libgsutils.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using testing::AnyOf;
using testing::Eq;

SC_MODULE(testA)
{
    void testA_method() {
        std::cout << "(cout) test A" << std::endl;
        GS_LOG("(GS_LOG) test A");
        SC_REPORT_INFO("testA","(SC_REPORT_INFO) test A");
    }
    SC_CTOR(testA)
    {
        GS_LOG("(GS_LOG) Construct TestA");
        SC_METHOD(testA_method);
    }
};

int sc_main(int argc, char **argv)
{
    std::stringstream buffer;
    std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());
    
    std::cout << "(cout) Logger test" << std::endl;
    GS_LOG("(GS_LOG) Logging test");
    SC_REPORT_INFO("sc_main","(SC_REPORT_INFO) Logging test");

    GS_LOG("(GS_LOG) instance testA");
    testA atest("TestA_Instance");
    GS_LOG("(GS_LOG) testA instanced");

    GS_LOG("(GS_LOG) running SC_START");
    sc_core::sc_start();
    GS_LOG("(GS_LOG) SC_START finished");

    std::cout.rdbuf(old);

    std::string text = buffer.str(); // text will now contain "Bla\n"
    std::cout << "Text found: " << std::endl << text << std::endl;
    
    return EXIT_SUCCESS;
}