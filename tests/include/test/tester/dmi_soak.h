/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs SAS
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

#ifndef TESTS_INCLUDE_TEST_TESTER_DMI_SOAK_H
#define TESTS_INCLUDE_TEST_TESTER_DMI_SOAK_H

#include <cstring>

#include "test/tester/mmio.h"


class CpuTesterDmiSoak : public CpuTesterMmio {
public:
    static const size_t NUM_BUF = 16;
    static constexpr uint64_t DMI_ADDR = 0x90000000;
    static constexpr uint64_t DMI_BITS = 10;
    static constexpr uint64_t DMI_SIZE = (1<<DMI_BITS)*NUM_BUF;
    static const uint64_t BANK_SIZE=(1<<DMI_BITS);

    enum SocketId {
        SOCKET_MMIO = CpuTesterMmio::SOCKET_MMIO,
        SOCKET_DMI,
    };

protected:

    

    /* 
     * We use two buffers and change the one in use after each complete DMI
     * invalidation. This allows to detect if a CPU is still wrongly using the
     * DMI pointer after invalidation. We could use mmap/mprotect to detect
     * this but the test would not build on non-POSIX systems.
     */
    uint8_t m_buf[NUM_BUF][BANK_SIZE];
    
    void dmi_b_transport(tlm::tlm_generic_payload &txn, sc_core::sc_time &delay)
    {
        uint64_t addr = txn.get_address() & (BANK_SIZE-1);
        uint64_t bank = (txn.get_address()>>DMI_BITS) & (NUM_BUF-1);
        uint64_t data = 0;
        uint64_t *ptr = reinterpret_cast<uint64_t *>(txn.get_data_ptr());
        size_t len = txn.get_data_length();

        GS_LOG("CPU access on DMI socket at 0x%" PRIx64 ", "
               "data: %" PRIu64 ", len: %zu", addr, data, len);

        if (len > 8) {
            TEST_FAIL("Unsupported b_transport data length");
        }

        TEST_ASSERT(addr + len <= BANK_SIZE);

        switch (txn.get_command()) {
            case tlm::TLM_READ_COMMAND:
                std::memcpy(ptr, &(m_buf[bank][addr]), len);
                m_cbs.mmio_read(SOCKET_DMI, addr, len);
                break;

            case tlm::TLM_WRITE_COMMAND:
                m_cbs.mmio_write(SOCKET_DMI, addr, *ptr, len);
                std::memcpy(&(m_buf[bank][addr]), ptr, len);
                break;

            default:
                TEST_FAIL("TLM command not supported");
        }

        txn.set_dmi_allowed(true);
        txn.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload &txn, tlm::tlm_dmi &dmi)
    {
        uint64_t addr = txn.get_address() & (BANK_SIZE-1);
        uint64_t bank = (txn.get_address()>>DMI_BITS) & (NUM_BUF-1);

        bool ret;

        dmi.set_start_address(bank<<DMI_BITS);
        dmi.set_end_address((bank<<DMI_BITS) + (BANK_SIZE - 1));
        dmi.allow_read_write();
        dmi.set_dmi_ptr(m_buf[bank]);

        /* Allow the test bench to override  */
        return m_cbs.dmi_request(SOCKET_DMI, txn.get_address(), txn.get_data_length(), dmi);

    }

public:
    tlm_utils::simple_target_socket<CpuTesterDmiSoak> dmi_socket;

    CpuTesterDmiSoak(const sc_core::sc_module_name &n, CpuTesterCallbackIface &cbs)
        : CpuTesterMmio(n, cbs)
    {
        dmi_socket.register_b_transport(this, &CpuTesterDmiSoak::dmi_b_transport);
        dmi_socket.register_get_direct_mem_ptr(this, &CpuTesterDmiSoak::get_direct_mem_ptr);

        m_cbs.map_target(dmi_socket, DMI_ADDR, DMI_SIZE);

        std::memset(m_buf, 0, sizeof(m_buf));
    }

    void dmi_invalidate(uint64_t start = 0, uint64_t end = DMI_SIZE - 1)
    {
         GS_LOG("Invalidating DMI 0x%" PRIx64 ", "
               " %" PRIu64 , start, end);
       dmi_socket->invalidate_direct_mem_ptr(start, end);
    }
};

#endif
