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

    CpuTesterMmio(const sc_core::sc_module_name& n, CpuTesterCallbackIface& cbs)
        : CpuTester(n, cbs) {
        register_b_transport(socket, SOCKET_MMIO);

        m_cbs.map_target(socket, MMIO_ADDR, MMIO_SIZE);
    }
};

#endif
