/*
 * Quic Module _MODULE_
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

#ifndef _MODULE__H
#define _MODULE__H

#include <systemc>
#include <cci_configuration>
#include <greensocs/gsutils/cciutils.h>
#include <scp/report.h>
#include <tlm>
#include <quic/gen_boilerplate/reg_model_maker.h>
#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/module_factory_registery.h>

#define _MODULE__REGS                              \
    /*  List all registers that are implemented */ \
    /*  (type, path, register ) */

class _MODULE_ : public virtual sc_core::sc_module
{
    SCP_LOGGER();
    cci::cci_broker_handle m_broker; // Should be a private broker for efficiency, but requires fixes to cci utils to
                                     // prevent use of cci_get_broker()
    gs::json_zip_archive m_jza;
    bool loaded_ok;
    gs::json_module M;
    GSREG_DECLARE(_MODULE__REGS);

public:
    tlm_utils::multi_passthrough_target_socket<_MODULE_> target_socket;

    SC_HAS_PROCESS(_MODULE_);
    _MODULE_(sc_core::sc_module_name name);

    void start_of_simulation()
    {
        /* NB writes should not happen before simulation starts,
         * Write any special register initial values here (e.g. over-right what is in the
         * configuration files)
         */
    }
    void before_end_of_elaboration()
    {
        SCP_TRACE(())("Before End of Elaboration, registering callbacks");
        /* Add callbacks tt the registers*/
        /* e.g. MYREG.post_write([&](TXN(txn)) {... my function...}); */
    }
};
#endif // _MODULE__H
