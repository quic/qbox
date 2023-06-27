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

#ifndef makenaau_core_top_csr_H
#define makenaau_core_top_csr_H

#include <systemc>
#include <cci_configuration>
#include <greensocs/gsutils/cciutils.h>
#include <scp/report.h>
#include <tlm>
#include <quic/gen_boilerplate/reg_model_maker.h>
#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/module_factory_registery.h>

#define makenaau_core_top_csr_REGS (uint32_t, tcsr_tcsr_regs.SOC_HW_VERSION, SOC_HW_VERSION)

/*  List all registers that are implemented */ /*  (type, path, register ) */

class makenaau_core_top_csr : public virtual sc_core::sc_module
{
    SCP_LOGGER();
    cci::cci_broker_handle m_broker; // Should be a private broker for efficiency, but requires fixes to cci utils to
                                     // prevent use of cci_get_broker()
    gs::json_zip_archive m_jza;
    bool loaded_ok;
    gs::json_module M;
    GSREG_DECLARE(makenaau_core_top_csr_REGS);

public:
    tlm_utils::multi_passthrough_target_socket<makenaau_core_top_csr> target_socket;

    SC_HAS_PROCESS(makenaau_core_top_csr);
    makenaau_core_top_csr(sc_core::sc_module_name name);

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

        SOC_HW_VERSION.post_read([&](TXN()) { SCP_DEBUG(())("Providing HW VERSION {:#x}", (uint64_t)SOC_HW_VERSION); });
    }
};
#endif // makenaau_core_top_csr_H
