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

#ifndef TESTS_INCLUDE_TEST_TESTER_EXCLUSIVE_H
#define TESTS_INCLUDE_TEST_TESTER_EXCLUSIVE_H

#include <greensocs/gsutils/tlm-extensions/exclusive-access.h>

#include <greensocs/base-components/misc/exclusive-monitor.h>

#include "test/tester/mmio.h"


class CpuTesterExclusive : public CpuTester {
public:
    static constexpr uint64_t MMIO_ADDR = 0x80000000;
    static constexpr size_t MMIO_SIZE = 1024;

    enum SocketId {
        SOCKET_MMIO = 0,
    };

protected:
    ExclusiveMonitor m_monitor;

public:
    TargetSocket socket;

    CpuTesterExclusive(const sc_core::sc_module_name &n, CpuTesterCallbackIface &cbs)
        : CpuTester(n, cbs)
        , m_monitor("exclusive-monitor")
    {
        register_b_transport(socket, SOCKET_MMIO);

        m_cbs.map_target(m_monitor.front_socket, MMIO_ADDR, MMIO_SIZE);
        socket.bind(m_monitor.back_socket);
    }

    void lock_region_64(uint64_t start)
    {
        using namespace tlm;

        tlm_generic_payload txn;
        ExclusiveAccessTlmExtension ext;
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        uint64_t buf;

        ext.add_hop(0x1337);

        txn.set_address(start);
        txn.set_data_ptr(reinterpret_cast<uint8_t *>(&buf));
        txn.set_data_length(sizeof(buf));
        txn.set_command(TLM_READ_COMMAND);
        txn.set_response_status(TLM_INCOMPLETE_RESPONSE);
        txn.set_extension(&ext);

        m_monitor.front_socket.get_base_export()->b_transport(txn, delay);

        txn.clear_extension(&ext);
    }

};

#endif

