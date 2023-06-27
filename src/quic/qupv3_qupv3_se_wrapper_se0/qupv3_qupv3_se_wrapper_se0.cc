/*
 * Quic Module qupv3_qupv3_se_wrapper_se0
 *
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

extern "C"

#define gen_xstr(s) _gen_str(s)
#define _gen_str(s) #s

#define INCBIN_SILENCE_BITCODE_WARNING
#include "quic/gen_boilerplate/incbin.h"

    INCBIN(ZipArchive_qupv3_qupv3_se_wrapper_se0_, __FILE__ "_config.zip");

#include <quic/gen_boilerplate/reg_model_maker.h>
#include "quic/qupv3_qupv3_se_wrapper_se0/qupv3_qupv3_se_wrapper_se0.h"

qupv3_qupv3_se_wrapper_se0::qupv3_qupv3_se_wrapper_se0(sc_core::sc_module_name _name)
    : sc_core::sc_module(_name)
    , m_broker("private_broker")
    , m_jza(zip_open_from_source(zip_source_buffer_create(gZipArchive_qupv3_qupv3_se_wrapper_se0_Data,
                                                          gZipArchive_qupv3_qupv3_se_wrapper_se0_Size, 0, nullptr),
                                 ZIP_RDONLY, nullptr))
    , loaded_ok(m_jza.json_read_cci(m_broker.create_broker_handle(), std::string(name()) + ".M"))
    , M("M", m_jza)
    , GSREG_CONSTRUCT(qupv3_qupv3_se_wrapper_se0_REGS)
    , target_socket("target_socket")
    , backend_socket("backend_socket")
    , irq("irq")
{
    SCP_TRACE(())("Constructor");
    sc_assert(loaded_ok);
    GSREG_BIND(qupv3_qupv3_se_wrapper_se0_REGS);
    target_socket.bind(M.target_socket);
    SC_METHOD(update_irq);
    sensitive << irq_update;
    backend_socket.register_b_transport(this, &qupv3_qupv3_se_wrapper_se0::receive);
}
