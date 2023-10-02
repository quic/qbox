#include <systemc>
#include <tlm>
#include <cci_configuration>
#include <scp/report.h>
#include "python_binder.h"
#include "router.h"
#include "memory.h"
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>
#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <greensocs/gsutils/ports/target-signal-socket.h>
#include <vector>
#include <sstream>
#include <memory>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <libgen.h>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

std::string getexepath()
{
    char result[1024] = { 0 };
#ifdef __APPLE__
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) != 0) {
        std::cerr << "error executing _NSGetExecutablePath() in python-binder-bench.h" << std::endl;
        exit(EXIT_FAILURE);
    }
    return std::string(dirname(result));
#elif __linux__
    ssize_t count = readlink("/proc/self/exe", result, 1024);
    if (count < 0) {
        std::cerr << "error executing readlink() in python-binder-bench.h: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return std::string(dirname(result), count);

#else
#error python-binder test supports only Mac OS and linux for now!
#endif
}

class PythonBinderTestBench : public TestBench
{
public:
    SCP_LOGGER();
    PythonBinderTestBench(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_initiator("initiator")
        , m_python_binder("python-binder")
        , m_router("router")
        , m_mem("mem")
        , m_test_initiator_signal_socket("test_initiator_signal_socket")
        , m_test_target_signal_socket("test_target_signal_socket")
    {
        SC_THREAD(do_signals_check);
        sensitive << signals_writer_event;
        dont_initialize();

        m_initiator.socket.bind(m_router.target_socket);
        m_python_binder.initiator_sockets[0].bind(m_router.target_socket);
        m_router.initiator_socket.bind(m_python_binder.target_sockets[0]);
        m_router.initiator_socket.bind(m_mem.socket);
        m_test_initiator_signal_socket.bind(m_python_binder.target_signal_sockets[0]);
        m_python_binder.initiator_signal_sockets[0].bind(m_test_target_signal_socket);
        m_test_target_signal_socket.register_value_changed_cb(
            [this](bool value) { m_signal_write_values.push_back(value); });
    }

    void do_basic_trans_check(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay, uint64_t addr, uint8_t* data,
                              uint8_t* r_data, size_t len, tlm::tlm_command command)
    {
        std::stringstream sstr;
        trans.set_address(addr);
        trans.set_data_length(len);
        trans.set_streaming_width(len);
        trans.set_data_ptr(data);
        trans.set_command(command);
        trans.set_response_status(tlm::tlm_response_status::TLM_INCOMPLETE_RESPONSE);
        m_initiator.set_next_txn_delay(delay + sc_core::sc_time(100, sc_core::sc_time_unit::SC_NS));

        if (command == tlm::tlm_command::TLM_WRITE_COMMAND) {
            sstr << "Written data: {";
            for (int i = 0; i < len; i++) {
                sstr << "0x" << std::hex << static_cast<uint64_t>(trans.get_data_ptr()[i])
                     << (i < (len - 1) ? "," : "");
            }
        }

        ASSERT_EQ(m_initiator.do_b_transport(trans), tlm::TLM_OK_RESPONSE);

        if (command == tlm::tlm_command::TLM_READ_COMMAND) {
            sstr << "Read data: {";
            for (int i = 0; i < len; i++) {
                ASSERT_EQ(trans.get_data_ptr()[i], r_data[i]);
                sstr << "0x" << std::hex << static_cast<uint64_t>(trans.get_data_ptr()[i])
                     << (i < (len - 1) ? "," : "");
            }
        }
        sstr << "}";
        SCP_INFO(()) << sstr.str();
    }

    void do_signals_check()
    {
        SCP_INFO(()) << "signals check test";
        for (int i = 0; i < 2; i++) {
            SCP_INFO(()) << "initiator signal write: true";
            m_test_initiator_signal_socket->write(true);
            sc_core::wait(sc_core::sc_time(sc_core::SC_ZERO_TIME));
            SCP_INFO(()) << "initiator signal write: false";
            m_test_initiator_signal_socket->write(false);
            sc_core::wait(sc_core::sc_time(sc_core::SC_ZERO_TIME));
        }
        std::vector<bool> test_signal_write_values = { false, true, false, true };
        ASSERT_EQ(m_signal_write_values, test_signal_write_values);
    }

    void print_dashes()
    {
        SCP_INFO(()) << "-----------------------------------------------------------------------------";
    }

protected:
    InitiatorTester m_initiator;
    gs::PythonBinder<> m_python_binder;
    gs::Router<> m_router;
    gs::Memory<> m_mem;
    InitiatorSignalSocket<bool> m_test_initiator_signal_socket;
    TargetSignalSocket<bool> m_test_target_signal_socket;
    std::vector<bool> m_signal_write_values;
    sc_core::sc_event signals_writer_event;
};