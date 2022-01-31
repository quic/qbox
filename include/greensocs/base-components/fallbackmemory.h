/*
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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

/* simple class to load csv file*/
class CSVRow {
public:
    std::string operator[](std::size_t index) const
    {
        return std::string(&m_line[m_data[index] + 1], m_data[index + 1] - (m_data[index] + 1));
    }
    std::size_t size() const
    {
        return m_data.size() - 1;
    }
    void readNextRow(std::istream& str)
    {
        std::getline(str, m_line);

        m_data.clear();
        m_data.emplace_back(-1);
        std::string::size_type pos = 0;
        while ((pos = m_line.find(',', pos)) != std::string::npos) {
            m_data.emplace_back(pos);
            ++pos;
        }
        // This checks for a trailing comma with no data after it.
        pos = m_line.size();
        m_data.emplace_back(pos);
    }

private:
    std::string m_line;
    std::vector<int> m_data;
};
std::istream& operator>>(std::istream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

/**
 * @class FallbackMemory
 *
 * @brief A FallbackMemory component that can add FallbackMemory to a virtual platform project
 *
 * @details This component models a FallbackMemory. It has a simple target socket so any other component with an initiator socket can connect to this component. It behaves as follows:
 *    - The FallbackMemory does not manage time in any way
 *    - It is only an LT model, and does not handle AT transactions
 *    - It does not manage exclusive accesses
 *    - You can manage the size of the FallbackMemory during the initialization of the component
 *    - FallbackMemory does not allocate individual "pages" but a single large
 *      block
 *    - It supports DMI requests with the method `get_direct_mem_ptr`
 *    - DMI invalidates are not issued.
 */
template <unsigned int BUSWIDTH = 32>
class FallbackMemory : public sc_core::sc_module {
private:
    /** Size of the FallbackMemory in bytes */
    uint64_t m_size;

    bool get_direct_mem_ptr(tlm::tlm_generic_payload& txn, tlm::tlm_dmi& dmi_data)
    {
        return false;
    }

protected:
    /** Host FallbackMemory where the data will be stored */
    uint8_t* m_ptr;

    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();

        if (txn.get_streaming_width() < len) {
            SC_REPORT_ERROR("FallbackMemory", "not supported.\n");
        }

        if ((addr + len) > m_size) {
            SC_REPORT_WARNING("FallbackMemory", "Request out of address bounds\n");
            txn.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }
        
        std::string access="";

        switch (txn.get_command()) {
        case tlm::TLM_READ_COMMAND:
            if (byt) {
                for (unsigned int i = 0; i < len; i++)
                    if (byt[i & bel] == TLM_BYTE_ENABLED)
                        ptr[i] = m_ptr[addr + i];
            } else {
                memcpy(ptr, &m_ptr[addr], len);
            }
            access="Read";
            break;
        case tlm::TLM_WRITE_COMMAND:
            if (byt) {
                for (unsigned int i = 0; i < len; i++)
                    if (byt[i & bel] == TLM_BYTE_ENABLED)
                        m_ptr[addr + i] = ptr[i];
            } else {
                memcpy(&m_ptr[addr], ptr, len);
            }
            access="Write";
            break;
        default:
            SC_REPORT_ERROR("FallbackMemory", "TLM command not supported\n");
            break;
        }

        SC_REPORT_WARNING("FallbackMemory", (access+" access to "+std::to_string(addr)+"\n").c_str());
        txn.set_response_status(tlm::TLM_OK_RESPONSE);

        txn.set_dmi_allowed(true);
    }

    unsigned int transport_dbg(tlm::tlm_generic_payload& txn)
    {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();

        if (txn.get_byte_enable_ptr() != 0 || txn.get_streaming_width() < len) {
            SC_REPORT_ERROR("FallbackMemory", "not supported.\n");
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
            SC_REPORT_ERROR("FallbackMemory", "TLM command not supported\n");
            break;
        }

        txn.set_response_status(tlm::TLM_OK_RESPONSE);

        return len;
    }

public:
    tlm_utils::simple_target_socket<FallbackMemory, BUSWIDTH> socket;

    FallbackMemory(sc_core::sc_module_name name, uint64_t size)
        : socket("socket")
    {
        m_size = size;

        m_ptr = new uint8_t[m_size]();

        socket.register_b_transport(this, &FallbackMemory::b_transport);
        socket.register_transport_dbg(this, &FallbackMemory::transport_dbg);
        socket.register_get_direct_mem_ptr(this, &FallbackMemory::get_direct_mem_ptr);
    }

    FallbackMemory() = delete;
    FallbackMemory(const FallbackMemory&) = delete;

    ~FallbackMemory()
    {
        delete[] m_ptr;
    }

    /**
     * @brief this function returns the size of the FallbackMemory
     *
     * @return the size of the FallbackMemory of type uint64_t
     */
    uint64_t size()
    {
        return m_size;
    }

    void csv_load(std::string filename, uint64_t offset, std::string addr_str, std::string value_str)
    {
        std::ifstream file(filename);

        CSVRow row;
        if (!(file >> row)) {
            SC_REPORT_ERROR("FallbackMemory",("Unable to find "+filename).c_str());
        }
        int addr_i=-1;
        int value_i=-1;
        for (int i=0; i<row.size(); i++) {
            if (row[i] == addr_str) {addr_i=i;}
            if (row[i] == value_str) {value_i=i;}
        }
        if (addr_i == -1 || value_i == -1) {
            SC_REPORT_ERROR("FallbackMemory", ("Unable to load "+filename).c_str());
        }
        while (file >> row) {
            const std::string addr=std::string(row[addr_i]);
            const std::string value=std::string(row[value_i]);            
            m_ptr[std::stoi(addr)]=std::stoi(value);
        }
    }

    /**
     * @brief This function reads a file into FallbackMemory and can be used to load an image.
     *
     * @param filename Name of the file
     * @param addr the address where the FallbackMemory file is to be read
     * @return size_t
     */
    size_t load(std::string filename, uint64_t addr)
    {
        std::ifstream fin(filename, std::ios::in | std::ios::binary);
        if (!fin.good()) {
            printf("FallbackMemory::load(): error file not found (%s)\n", filename.c_str());
            exit(1);
        }
        fin.read(reinterpret_cast<char*>(&m_ptr[addr]), m_size);
        size_t r = fin.gcount();
        fin.close();
        return r;
    }

    /**
     * @brief copy an existing image in host FallbackMemory into the modelled FallbackMemory
     *
     * @param ptr Pointer to the FallbackMemory
     * @param len Length of the read
     * @param addr Address of the read
     */
    void load(const uint8_t* ptr, uint64_t len, uint64_t addr)
    {
        memcpy(&m_ptr[addr], ptr, len);
    }
};
