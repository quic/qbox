/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "dmi-converter.h"
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>
#include <vector>
#include <sstream>
#include <memory>
#include <cstring>

#define MEM_SIZE       4096
#define MIN_ALLOC_UNIT 8

template <unsigned int BUSWIDTH = 32>
class SimpleMemory : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    using MOD = SimpleMemory<BUSWIDTH>;
    using tlm_target_socket_t = tlm_utils::simple_target_socket_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                  sc_core::SC_ZERO_OR_MORE_BOUND>;
    tlm_target_socket_t target_socket;

    SimpleMemory(const sc_core::sc_module_name& nm): sc_core::sc_module(nm), m_mem(new unsigned char[MEM_SIZE])
    {
        target_socket.register_b_transport(this, &SimpleMemory::b_transport);
        target_socket.register_get_direct_mem_ptr(this, &SimpleMemory::get_direct_mem_ptr);
        clear();
    }
    ~SimpleMemory() { delete[] m_mem; }

private:
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        sc_dt::uint64 addr = trans.get_address();
        tlm::tlm_command cmd = trans.get_command();
        unsigned char* data = trans.get_data_ptr();
        unsigned int len = trans.get_data_length();
        unsigned char* byt = trans.get_byte_enable_ptr();
        unsigned int bel = trans.get_byte_enable_length();

        if ((addr + len) > MEM_SIZE) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        switch (cmd) {
        case tlm::TLM_WRITE_COMMAND:
            if (byt) {
                for (int i = 0; i < len; i++) {
                    if (byt[i % bel] == TLM_BYTE_ENABLED) m_mem[addr + i] = data[i];
                }
            } else
                memcpy(&m_mem[addr], data, len);
            break;
        case tlm::TLM_READ_COMMAND:
            if (byt) {
                for (int i = 0; i < len; i++) {
                    if (byt[i % bel] == TLM_BYTE_ENABLED) data[i] = m_mem[addr + i];
                }
            } else
                memcpy(data, &m_mem[addr], len);
            break;
        case tlm::TLM_IGNORE_COMMAND:
            return;
        default:
            SCP_FATAL(()) << "invalid tlm_command at address: 0x" << std::hex << addr;
        }
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    /**
     * Grant read/write DMI access if address is in range: 0 - MEM_SIZE/2.
     * The DMI granted adress range is allocated in MIN_ALLOC_UNIT bytes chunks.
     */

    bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        sc_dt::uint64 addr = trans.get_address();
        tlm::tlm_command cmd = trans.get_command();
        unsigned char* data = trans.get_data_ptr();
        unsigned int len = trans.get_data_length();

        if ((addr + len - 1) > MEM_SIZE) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return false;
        } else if ((addr + len - 1) >= (3 * MEM_SIZE / 4) && (addr + len - 1) < MEM_SIZE) {
            trans.set_dmi_allowed(false);
            dmi_data.allow_none();
            return false;
        } else if ((addr + len - 1) >= (MEM_SIZE / 2) && (addr + len - 1) < (3 * MEM_SIZE / 4)) {
            dmi_data.allow_read();
        } else if ((addr + len - 1) < (MEM_SIZE / 2)) {
            dmi_data.allow_read_write();
        }

        trans.set_dmi_allowed(true);
        sc_dt::uint64 start_addr = static_cast<uint64_t>(addr / MIN_ALLOC_UNIT) * MIN_ALLOC_UNIT;
        sc_dt::uint64 end_addr = (static_cast<uint64_t>((addr + len - 1) / MIN_ALLOC_UNIT) + 1) * MIN_ALLOC_UNIT - 1;
        dmi_data.set_start_address(start_addr);
        dmi_data.set_end_address(end_addr);
        dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char*>(&m_mem[start_addr]));
        return true;
    }

public:
    uint8_t read_byte(uint64_t addr) const { return static_cast<uint8_t>(m_mem[addr]); }

    void clear() { std::memset(m_mem, 0, MEM_SIZE); }

private:
    unsigned char* m_mem;
};

class DMIConverterTestBench : public TestBench
{
public:
    SCP_LOGGER();
    DMIConverterTestBench(const sc_core::sc_module_name& n)
        : TestBench(n), m_initiator("initiator"), m_simple_mem("SimpleMemory"), m_dmi_converter("DMIConverter")
    {
        m_initiator.socket.bind(m_dmi_converter.target_sockets[0]);
        m_dmi_converter.initiator_sockets[0].bind(m_simple_mem.target_socket);
    }

    void do_write_read_check(uint64_t addr, uint8_t* w_data, size_t len)
    {
        std::stringstream sstr;
        std::unique_ptr<uint8_t[]> r_data{ new uint8_t[len] };
        ASSERT_EQ(m_initiator.do_write_with_ptr(addr, w_data, len, false), tlm::TLM_OK_RESPONSE);
        sstr << "Written data: {";
        for (int i = 0; i < len; i++) {
            ASSERT_EQ(m_simple_mem.read_byte(addr + i), w_data[i]);
            sstr << "0x" << std::hex << static_cast<uint64_t>(m_simple_mem.read_byte(addr + i))
                 << (i < (len - 1) ? "," : "");
        }
        sstr << "}";
        SCP_INFO(()) << sstr.str();
        sstr.str("");

        ASSERT_EQ(m_initiator.do_read_with_ptr(addr, r_data.get(), len, false), tlm::TLM_OK_RESPONSE);
        sstr << "Read data: {";
        for (int i = 0; i < len; i++) {
            ASSERT_EQ(m_simple_mem.read_byte(addr + i), r_data[i]);
            sstr << "0x" << std::hex << static_cast<uint64_t>(r_data[i]) << (i < (len - 1) ? "," : "");
        }
        sstr << "}";
        SCP_INFO(()) << sstr.str();
    }

    void do_write_read_check_be(tlm::tlm_generic_payload& trans, uint64_t addr, uint8_t* w_data, size_t len,
                                uint8_t* byt, size_t bel)
    {
        std::stringstream sstr;
        std::unique_ptr<uint8_t[]> r_data{ new uint8_t[len] };
        std::unique_ptr<uint8_t[]> ref_byt{ new uint8_t[bel] };
        std::memset(r_data.get(), 0, len);
        std::memcpy(ref_byt.get(), byt, bel);
        m_simple_mem.clear();
        trans.set_address(addr);
        trans.set_data_length(len);
        trans.set_data_ptr(w_data);
        trans.set_byte_enable_length(bel);
        trans.set_byte_enable_ptr(byt);
        trans.set_command(tlm::TLM_WRITE_COMMAND);
        ASSERT_EQ(m_initiator.do_b_transport(trans), tlm::TLM_OK_RESPONSE);
        sstr << "Written data: {";
        for (int i = 0; i < len; i++) {
            ASSERT_EQ(m_simple_mem.read_byte(addr + i), w_data[i] & ref_byt[i % bel]);
            sstr << "0x" << std::hex << static_cast<uint64_t>(m_simple_mem.read_byte(addr + i))
                 << (i < (len - 1) ? "," : "");
        }
        sstr << "}";
        SCP_INFO(()) << sstr.str();
        sstr.str("");

        sstr << "Byte Enable:  {";
        for (int i = 0; i < len; i++) {
            sstr << "0x" << std::hex << static_cast<uint64_t>(ref_byt[i % bel]) << (i < (len - 1) ? "," : "");
        }
        sstr << "}";
        SCP_INFO(()) << sstr.str();
        sstr.str("");

        // Assuming that other trans data members are not mutated by the memory
        trans.set_data_ptr(r_data.get());
        trans.set_command(tlm::TLM_READ_COMMAND);
        ASSERT_EQ(m_initiator.do_b_transport(trans), tlm::TLM_OK_RESPONSE);
        sstr << "Read data:    {";
        for (int i = 0; i < len; i++) {
            ASSERT_EQ(r_data[i], w_data[i] & ref_byt[i % bel]);
            sstr << "0x" << std::hex << static_cast<uint64_t>(r_data[i]) << (i < (len - 1) ? "," : "");
        }
        sstr << "}";
        SCP_INFO(()) << sstr.str();
    }

    void print_dashes()
    {
        SCP_INFO(()) << "-----------------------------------------------------------------------------";
    }

protected:
    InitiatorTester m_initiator;
    ::SimpleMemory<> m_simple_mem;
    gs::DMIConverter<> m_dmi_converter;
};