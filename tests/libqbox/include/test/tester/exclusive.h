/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TESTS_INCLUDE_TEST_TESTER_EXCLUSIVE_H
#define TESTS_INCLUDE_TEST_TESTER_EXCLUSIVE_H

#include <tlm-extensions/exclusive-access.h>

#include <exclusive-monitor.h>
#include <tlm-extensions/pathid_extension.h>

#include "test/tester/mmio.h"

class CpuTesterExclusive : public CpuTester
{
public:
    static constexpr uint64_t MMIO_ADDR = 0x80000000;
    static constexpr size_t MMIO_SIZE = 1024;

    enum SocketId {
        SOCKET_MMIO = 0,
    };

protected:
    exclusive_monitor m_monitor;

public:
    TargetSocket socket;

    CpuTesterExclusive(const sc_core::sc_module_name& n, CpuTesterCallbackIface& cbs)
        : CpuTester(n, cbs), m_monitor("exclusive-monitor")
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
        gs::PathIDExtension pid;
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        uint64_t buf;

        ext.add_hop(0x1337);
        pid.push_back(0x1337);

        txn.set_address(start);
        txn.set_data_ptr(reinterpret_cast<uint8_t*>(&buf));
        txn.set_data_length(sizeof(buf));
        txn.set_command(TLM_READ_COMMAND);
        txn.set_response_status(TLM_INCOMPLETE_RESPONSE);
        txn.set_extension(&ext);
        txn.set_extension(&pid);

        m_monitor.front_socket.get_base_export()->b_transport(txn, delay);

        txn.clear_extension(&ext);
        txn.clear_extension(&pid);
    }
};

#endif
