/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <fstream>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#ifndef _WIN32
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

class Memory : public sc_core::sc_module {
private:
    /** Size of the memory in bytes */
    uint64_t m_size;

    /** Host memory where the data will be stored */
    uint8_t *m_ptr;

    /** Wether the current ptr has been mapped to a file */
    bool m_mapped;

    void b_transport(tlm::tlm_generic_payload &txn, sc_core::sc_time &delay) {
        unsigned int len = txn.get_data_length();
        unsigned char *ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();

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

    unsigned int transport_dbg(tlm::tlm_generic_payload &txn) {
        unsigned int len = txn.get_data_length();
        unsigned char *ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();

        if (txn.get_byte_enable_ptr() != 0 || txn.get_streaming_width() < len) {
            SC_REPORT_ERROR("Memory", "not supported.\n");
        }

        if ((addr + len) > m_size) {
            txn.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return 0;
        }

        switch (txn.get_command()) {
            case tlm::TLM_READ_COMMAND:
                memcpy(ptr, &m_ptr[addr], len);
                break;
            case tlm::TLM_WRITE_COMMAND:
                memcpy(&m_ptr[addr], ptr, len);
                break;
            default:
                len = 0;
                SC_REPORT_ERROR("Memory", "TLM command not supported\n");
                break;
        }

        txn.set_response_status(tlm::TLM_OK_RESPONSE);

        return len;
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload &txn, tlm::tlm_dmi &dmi_data) {
        static const sc_core::sc_time LATENCY(10, sc_core::SC_NS);

        dmi_data.allow_read_write();
        dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char *>(&m_ptr[0]));
        dmi_data.set_start_address(0);
        dmi_data.set_end_address(m_size - 1);
        dmi_data.set_read_latency(LATENCY);
        dmi_data.set_write_latency(LATENCY);

        return true;
    }

public:
    tlm_utils::simple_target_socket<Memory> socket;

    Memory(sc_core::sc_module_name name, uint64_t size)
        : socket("socket"), m_mapped(false)
    {
        m_size = size;

        m_ptr = new uint8_t[m_size]();

        socket.register_b_transport(this, &Memory::b_transport);
        socket.register_transport_dbg(this, &Memory::transport_dbg);
        socket.register_get_direct_mem_ptr(this, &Memory::get_direct_mem_ptr);
    }

    Memory() = delete;
    Memory(const Memory&) = delete;

    ~Memory() {
        if (!m_mapped)
            delete[] m_ptr;
    }

    uint64_t size() {
        return m_size;
    }

    void map(const char *filename) {
#ifdef _WIN32
        int fd = open(filename, O_RDWR);
        if (fd < 0) {
            SC_REPORT_ERROR("Memory", "Unable to find backing file\n");
        }
        if (m_ptr && !m_mapped) {
            delete[] m_ptr;
        }
        m_mapped = true;
        m_ptr = (uint8_t *)mmap(m_ptr, m_size, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, fd, 0);
        if (m_ptr == MAP_FAILED) {
            SC_REPORT_ERROR("Memory", "Unable to map backing file\n");
        }
#else
        SC_REPORT_ERROR("Memory", "Backing files only supported on UNIX platforms\n");
#endif
    }
    size_t load(std::string filename, uint64_t addr) {
        std::ifstream fin(filename, std::ios::in | std::ios::binary);
        if (!fin.good()) {
            printf("Memory::load(): error file not found (%s)\n", filename.c_str());
            exit(1);
        }
        return fin.readsome((char *) &m_ptr[addr], m_size);
    }

    void load(const uint8_t *ptr, uint64_t len, uint64_t addr) {
        memcpy(&m_ptr[addr], ptr, len);
    }
};
