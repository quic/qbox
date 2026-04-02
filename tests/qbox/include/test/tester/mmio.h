/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TESTS_INCLUDE_TEST_TESTER_MMIO_H
#define TESTS_INCLUDE_TEST_TESTER_MMIO_H

#include "test/tester/tester.h"

class CpuTesterMmio : public CpuTester
{
public:
    static constexpr uint64_t MMIO_ADDR = 0x80000000;
    static constexpr size_t MMIO_SIZE = 1024;

    enum SocketId {
        SOCKET_MMIO = 0,
    };

public:
    TargetSocket socket;

    CpuTesterMmio(const sc_core::sc_module_name& n, CpuTesterCallbackIface& cbs): CpuTester(n, cbs)
    {
        register_b_transport(socket, SOCKET_MMIO);

        m_cbs.map_target(socket, MMIO_ADDR, MMIO_SIZE);
    }
};

#endif
