/*
 * Quic Module makenaau_core_top_csr
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

    INCBIN(ZipArchive_makenaau_core_top_csr_, __FILE__ "_config.zip");

#include <quic/gen_boilerplate/reg_model_maker.h>
#include "quic/makenaau_core_top_csr/makenaau_core_top_csr.h"

makenaau_core_top_csr::makenaau_core_top_csr(sc_core::sc_module_name _name)
    : sc_core::sc_module(_name)
    , m_broker(cci::cci_get_broker())
    , m_jza(zip_open_from_source(zip_source_buffer_create(gZipArchive_makenaau_core_top_csr_Data,
                                                          gZipArchive_makenaau_core_top_csr_Size, 0, nullptr),
                                 ZIP_RDONLY, nullptr))
    , loaded_ok(m_jza.json_read_cci(m_broker.create_broker_handle(), std::string(name()) + ".M"))
    , M("M", m_jza)
    , GSREG_CONSTRUCT(makenaau_core_top_csr_REGS)
    , target_socket("target_socket")
{
    SCP_TRACE(())("Constructor");
    sc_assert(loaded_ok);
    GSREG_BIND(makenaau_core_top_csr_REGS);

    target_socket.bind(M.target_socket);
}
