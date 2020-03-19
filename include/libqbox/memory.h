#pragma once

#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

class Memory : public sc_core::sc_module
{
private:
    /** Size of the memory in bytes */
    uint64_t m_size;

    /** Host memory where the data will be stored */
    uint8_t *m_ptr;

public:
    tlm_utils::simple_target_socket<Memory> socket;

#if 0
    sc_core::sc_in <bool>  clk_in;
#endif

    Memory(sc_core::sc_module_name name, uint64_t size)
        : socket("socket")
    {
        m_size = size;

        m_ptr = new uint8_t[m_size]();

        socket.register_b_transport(this, &Memory::b_transport);

        socket.register_get_direct_mem_ptr(this, &Memory::get_direct_mem_ptr);
    }

    ~Memory()
    {
        delete[] m_ptr;
    }

    uint64_t size()
    {
        return m_size;
    }

    void load(std::string filename, uint64_t addr)
    {
        std::ifstream fin(filename, std::ios::in | std::ios::binary);
        fin.read((char *)&m_ptr[addr], m_size);
    }

    void load(const uint8_t *ptr, uint64_t len, uint64_t addr)
    {
        memcpy(&m_ptr[addr], ptr, len);
    }

private:
    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        unsigned int len = txn.get_data_length();
        unsigned char *ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();

#if 0
        sc_core::wait(sc_core::sc_time(1, sc_core::SC_SEC));
        sc_core::wait(clk_in.posedge_event());
#endif

        if (txn.get_byte_enable_ptr() != 0 || txn.get_streaming_width() < len) {
            SC_REPORT_ERROR("Memory", "not supported.\n");
        }

        if ((addr + len) > m_size) {
            txn.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        switch (txn.get_command()) {
        case tlm::TLM_READ_COMMAND:
            memcpy(ptr, &m_ptr[addr], len);
            break;
        case tlm::TLM_WRITE_COMMAND:
            memcpy(&m_ptr[addr], ptr, len);
            break;
        default:
            SC_REPORT_ERROR("Memory", "TLM command not supported\n");
            break;
        }

        txn.set_response_status(tlm::TLM_OK_RESPONSE);

        txn.set_dmi_allowed(true);
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload& txn, tlm::tlm_dmi& dmi_data)
    {
        static const sc_core::sc_time LATENCY(10, sc_core::SC_NS);

        dmi_data.allow_read_write();
        dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char *>(&m_ptr[0]));
        dmi_data.set_start_address(0);
        dmi_data.set_end_address(m_size - 1);
        dmi_data.set_read_latency(LATENCY);
        dmi_data.set_write_latency(LATENCY);

        return true;
    }
};
