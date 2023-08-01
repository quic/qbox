/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef __TRANSACTION_FORWARD_IF__
#define __TRANSACTION_FORWARD_IF__

#include <systemc>
#include <tlm>

namespace gs {

enum ForwarderType {
    CONTAINER /*modules container*/,
    PASS /*PassRPC model to work in LOCAL or REMOTE mode*/,
    SC_MODULE /*Generic sc_module*/
};

template <ForwarderType E>
class transaction_forwarder_if
{
public:
    virtual void fw_b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) = 0;

    virtual unsigned int fw_transport_dbg(int id, tlm::tlm_generic_payload& trans) = 0;

    virtual bool fw_get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) = 0;

    virtual void fw_invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) = 0;

    virtual void fw_handle_signal(int id, bool value) = 0;

    virtual ~transaction_forwarder_if() = default;
};
} // namespace gs
#endif