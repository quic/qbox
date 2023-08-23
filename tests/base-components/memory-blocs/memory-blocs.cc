#include <systemc>
#include <tlm>
#include <scp/report.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "memory.h"
#include "loader.h"
#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/tests/initiator-tester.h>

class MemoryBlocs : public sc_core::sc_module
{
public:
    // static constexpr size_t MEMORY_SIZE = 0x1000;

    void test_write_one_bloc()
    {
        int tab[0x200];
        // int tab = 0xFF;
        int data1[0x200];
        uint64_t data;

        for (int i = 0; i < 0x200; i++) {
            tab[i] = 4;
        }

        m_initiator.do_write(0x450, tab);
        m_initiator.do_read(0x450, data1);

        for (int i = 0; i < 0x200; i++) {
            assert(data1[i] == tab[i]);
        }

        m_initiator.do_write<uint8_t>(0x10000, 0xFF);
        m_initiator.do_read(0x10000, data);
        assert(0xFF == data);

        m_initiator.do_write<uint16_t>(0x20000, 0xFF);
        m_initiator.do_read(0x20000, data);
        assert(0xFF == data);

        m_initiator.do_write<uint32_t>(0x30000, 0xFF);
        m_initiator.do_read(0x30000, data);
        assert(0xFF == data);

        m_initiator.do_write<uint64_t>(0x50000, 0xFF);
        m_initiator.do_read(0x50000, data);
        assert(0xFF == data);
    }

    void test_write_two_blocs()
    {
        int tab[0x80000];
        int data1[0x80000];
        uint8_t data;

        for (int i = 0; i < 0x80000; i++) {
            tab[i] = 4;
        }

        m_initiator.do_write(0x33FFFFF00, tab);
        m_initiator.do_read(0x33FFFFF00, data1);

        for (int i = 0; i < 0x80000; i++) {
            assert(data1[i] == tab[i]);
            // SCP_INFO(SCMOD) << " i= "<< i << endl;
            // SCP_INFO(SCMOD) << "data1 = "<< data1[i] << endl;
            // SCP_INFO(SCMOD) << "tab = "<< tab[i] << endl;
        }

        m_initiator.do_write<uint8_t>(0x400000000, 0xFF);
        m_initiator.do_read(0x400000000, data);
        assert(0xFF == data);
    }

    void test_write_debug_one_bloc()
    {
        int tab[0x200];
        int data1[0x200];
        uint64_t data;

        for (int i = 0; i < 0x200; i++) {
            tab[i] = 4;
        }

        m_initiator.do_write(0x100000, tab, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(tab));
        m_initiator.do_read(0x100000, data1, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(data1));

        for (int i = 0; i < 0x200; i++) {
            assert(data1[i] == tab[i]);
        }

        m_initiator.do_write<uint8_t>(0x200000, 0xFF, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(uint8_t));
        m_initiator.do_read(0x10000, data, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(data));
        assert(0xFF == data);

        m_initiator.do_write<uint16_t>(0x300000, 0xFF, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(uint16_t));
        m_initiator.do_read(0x20000, data, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(data));
        assert(0xFF == data);

        m_initiator.do_write<uint32_t>(0x400000, 0xFF, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(uint32_t));
        m_initiator.do_read(0x30000, data, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(data));
        assert(0xFF == data);

        m_initiator.do_write<uint64_t>(0x500000, 0xFF, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(uint64_t));
        m_initiator.do_read(0x50000, data, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(data));
        assert(0xFF == data);
    }

    void test_write_debug_two_blocs()
    {
        int tab[0x200];
        int data[0x200];

        for (int i = 0; i < 0x200; i++) {
            tab[i] = 8;
        }

        m_initiator.do_write(0x33FFFFF00, tab, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(tab));
        m_initiator.do_read(0x33FFFFF00, data, true);
        assert(m_initiator.get_last_transport_debug_ret() == sizeof(data));

        for (int i = 0; i < 0x200; i++) {
            assert(data[i] == tab[i]);
        }
    }

    void do_good_dmi_request_and_check(uint64_t addr)
    {
        using namespace tlm;

        bool ret = m_initiator.do_dmi_request(addr);

        assert(ret);
    }

    void dmi_write_or_read(uint64_t addr, uint8_t* data, size_t data_len, bool is_read)
    {
        using namespace tlm;

        uint64_t len = 0;
        uint64_t data_ptr_index = 0;

        while (data_len > 0) {
            do_good_dmi_request_and_check(addr);
            const tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();

            uint64_t start_addr = dmi_data.get_start_address();
            uint64_t end_addr = dmi_data.get_end_address();
            uint64_t bloc_size = (end_addr - start_addr) + 1;

            uint64_t bloc_offset = addr - start_addr;
            uint64_t bloc_len = bloc_size - bloc_offset;
            len = (data_len < bloc_len) ? data_len : bloc_len;

            if (is_read == TLM_READ_COMMAND) {
                assert(dmi_data.is_read_allowed());
                memcpy(&data[data_ptr_index], dmi_data.get_dmi_ptr() + bloc_offset, len);
            } else {
                assert(dmi_data.is_write_allowed());
                memcpy(dmi_data.get_dmi_ptr() + bloc_offset, &data[data_ptr_index], len);
            }

            data_ptr_index += len;
            data_len -= len;
            addr += len;
        }
    }

    void test_write_with_DMI()
    {
        uint8_t data = 0xFF;
        uint8_t tab[0x80000];
        uint8_t data1[0x80000];
        uint8_t data_read;

        for (int i = 0; i < 0x80000; i++) {
            tab[i] = 8;
        }

        /* Simple send/read data block 0*/
        dmi_write_or_read(0x100, &data, sizeof(data), true);
        dmi_write_or_read(0x100, &data_read, sizeof(data), false);
        assert(data == data_read);

        data = 0x04;
        /* Simple send/read data block 1 */
        dmi_write_or_read(0x400000000, &data, sizeof(data), true);
        dmi_write_or_read(0x400000000, &data_read, sizeof(data), false);
        assert(data == data_read);

        dmi_write_or_read(0x33FFFFF00, tab, sizeof(tab), true);
        dmi_write_or_read(0x33FFFFF00, data1, sizeof(data1), false);

        for (int i = 0; i < 0x80000; i++) {
            assert(data1[i] == tab[i]);
        }
    }

    template <class T>
    void test_rw()
    {
        T seed = 0x11;
        for (int i = 0; i < sizeof(T); i++) seed += seed << 8;
        for (int i = 1; i < 30; i++) {
            for (int a = 0; a < 16; a++) {
                uint64_t addr = ((i * 0x1000) - (8 * sizeof(T))) + (a * sizeof(T));
                assert(m_initiator.do_write(addr, seed * a, false) == tlm::TLM_OK_RESPONSE);
            }
        }
        for (int i = 1; i < 3; i++) {
            for (int a = 0; a < 16; a++) {
                T d;
                uint64_t addr = ((i * 0x1000) - (8 * sizeof(T))) + (a * sizeof(T));
                assert(m_initiator.do_read(addr, d, false) == tlm::TLM_OK_RESPONSE);
                assert(d == (seed * a));
            }
        }
    }

    template <class T>
    void test_rdmiw()
    {
        T seed = 0x11;
        uint8_t* old_ptr;
        int boundry = 0;
        for (int i = 0; i < sizeof(T); i++) seed += seed << 8; // make a fairly random data thats different in each byte
        for (int i = 1; i < 30; i++) {
            old_ptr = 0;
            for (int a = 0; a < 16; a++) {
                uint64_t addr = ((i * 0x1000) - (8 * sizeof(T))) + (a * sizeof(T));

                do_good_dmi_request_and_check(addr);
                const tlm::tlm_dmi& dmi_data = m_initiator.get_last_dmi_data();
                uint64_t start_addr = dmi_data.get_start_address();
                uint64_t end_addr = dmi_data.get_end_address();
                uint8_t* ptr = dmi_data.get_dmi_ptr();
                if (old_ptr != ptr) {
                    assert(a == 0 || a == 8);
                    if (a == 8) {
                        boundry++;
                    }
                }
                old_ptr = ptr;
                assert(start_addr <= addr);
                assert(addr <= end_addr);
                T data = seed * a;
                memcpy(dmi_data.get_dmi_ptr() + (addr - start_addr), &data, sizeof(T));
            }
        }
        for (int i = 1; i < 3; i++) {
            for (int a = 0; a < 16; a++) {
                T d;
                uint64_t addr = ((i * 0x1000) - (8 * sizeof(T))) + (a * sizeof(T));
                assert(m_initiator.do_read(addr, d, false) == tlm::TLM_OK_RESPONSE);
                assert(d == (seed * a));
            }
        }
        assert(boundry > 0);
    }

protected:
    gs::ConfigurableBroker m_broker;
    InitiatorTester m_initiator;
    gs::Memory<> m_memory;

public:
    MemoryBlocs(const sc_core::sc_module_name& n)
        : sc_core::sc_module(n), m_broker(), m_initiator("initiator"), m_memory("memory")
    {
        m_initiator.socket.bind(m_memory.socket);
    }

    virtual ~MemoryBlocs() {}
};

int sc_main(int argc, char* argv[])
{
    static constexpr size_t MEMORY_SIZE = 0xD00000000;

    auto m_broker = new gs::ConfigurableBroker(
        argc, argv,
        {
            { "test.memory.min_block_size", cci::cci_value(0x1000) },
            { "test.memory.max_block_size", cci::cci_value(0x10000) },
            { "test.memory.target_socket.size", cci::cci_value(MEMORY_SIZE) },
            { "test.memory.target_socket.address", cci::cci_value(0) },
            { "test.memory.target_socket.relative_addresses", cci::cci_value(true) },
        });

    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(10));      // make the msg type column a bit tighter

    MemoryBlocs test("test");

    sc_core::sc_start();

    test.test_write_one_bloc();
    test.test_write_two_blocs();
    test.test_write_debug_one_bloc();
    test.test_write_debug_two_blocs();
    test.test_write_with_DMI();

    test.test_rdmiw<uint8_t>();
    test.test_rdmiw<uint64_t>();
    test.test_rw<uint8_t>();
    test.test_rw<uint64_t>();
    return 0;
}