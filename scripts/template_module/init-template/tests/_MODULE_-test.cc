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

#include <greensocs/base-components/router.h>
#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/reg_model_maker/reg_model_maker.h>

#include "quic/_MODULE_/_MODULE_.h"

SC_MODULE (example) {
    SCP_LOGGER();
    SC_CTOR (example) {
        SCP_DEBUG(())("Constructor");
        auto broker = cci::cci_get_broker();

        std::vector<gs::gs_register_if*> regs = gs::all_registers();
        for (auto reg : regs) {
            auto r = reg->create_param_handle();
            SCP_INFO(())("Setting up callback for {}", r.name());
            r.register_pre_read_callback(
                [this, r](const cci::cci_param_read_event<void>& ev) {
                    SCP_INFO(())
                    ("Pre read access to : {} (value {})", r.name(), r.get_cci_value().to_json());
                },
                cci::cci_untyped_tag());
        }
    }
};

SC_MODULE (top) {
    SCP_LOGGER();
    gs::Router<> m_router;
    _MODULE_ m_regs;
    InitiatorTester m_initiator;
    example m_example;

    SC_HAS_PROCESS();

    top(sc_core::sc_module_name _name)
        : sc_core::sc_module(_name)
        , m_router("router")
        , m_regs("test_regs")
        , m_initiator("init")
        , m_example("example")
    {
        SCP_INFO(())("TOP Constructor");

        m_initiator.socket(m_router.target_socket);
        m_router.initiator_socket(m_regs.target_socket);
        SC_THREAD(reg_access);
    }

    template <class T>
    static struct std::vector<T*> objs_in(sc_core::sc_object * m) {
        std::vector<T*> res;
        for (auto c : m->get_child_objects()) {
            if (T* r = dynamic_cast<T*>(c)) {
                res.push_back(r);
            }
        }
        return res;
    };
    void reg_mod_access(sc_core::sc_module * mod, uint64_t base)
    {
        auto m_broker = cci::cci_get_broker();

        uint64_t mod_base = 0;
        if (m_broker.has_preset_value(std::string(mod->name()) + ".target_socket.address")) {
            m_broker.get_preset_cci_value(std::string(mod->name()) + ".target_socket.address")
                .template try_get<uint64_t>(mod_base);
        } else {
            mod_base = base;
        }

        std::vector<sc_core::sc_module*> mods = objs_in<sc_core::sc_module>(mod);
        for (auto m : mods) {
            wait(1, sc_core::SC_NS);
            SCP_INFO(())("Mod: {} 0x{:x}", m->name(), mod_base);
            reg_mod_access(m, mod_base);
        }

        std::vector<gs::gs_register_if*> regs = objs_in<gs::gs_register_if>(mod);
        for (auto r : regs) {
            wait(1, sc_core::SC_NS);

            uint32_t value = 0xf00dfadd;
            SCP_INFO(())
            ("Read from 0x{:x} ( {}  :  {} )", r->get_offset() + mod_base, mod->name(), r->get_cci_name());
            if (m_initiator.do_read(r->get_offset() + mod_base, value) != tlm::TLM_OK_RESPONSE) {
                SCP_FATAL(())
                ("UNABLE to read from 0x{:x} ( {}  :  {} )", r->get_offset() + mod_base, mod->name(),
                 r->get_cci_name());
            }
            SCP_INFO(())
            ("Read from 0x{:x}  value 0x{:x}", r->get_offset() + mod_base, value);
        }
    }
    void reg_access()
    {
        std::vector<sc_core::sc_module*> mods = objs_in<sc_core::sc_module>(gs::find_sc_obj(nullptr, m_regs.name()));
        for (auto m : mods) {
            wait(1, sc_core::SC_NS);
            SCP_INFO(()) << "Mod: " << m->name();
            reg_mod_access(m, 0);
        }
    }
};

int sc_main(int argc, char* argv[])
{
    scp::init_logging(scp::LogConfig()
                          .fileInfoFrom(sc_core::SC_ERROR)
                          .logAsync(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(50));      // make the msg type column a bit tighter
    gs::ConfigurableBroker m_broker(argc, argv, { { "log_level", cci::cci_value(5) } });

    top m_top("top");
    auto start = std::chrono::system_clock::now();
    try {
        SCP_INFO() << "SC_START";
        sc_core::sc_start();
    } catch (std::runtime_error const& e) {
        std::cerr << argv[0] << "Error: '" << e.what() << std::endl;
        exit(1);
    } catch (const std::exception& exc) {
        std::cerr << argv[0] << " Error: '" << exc.what() << std::endl;
        exit(2);
    } catch (...) {
        SCP_ERR() << "Unknown error (main.cc)!";
        exit(3);
    }

    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    SCP_INFO() << "Simulation duration: " << elapsed.count() << "s\n";
    return 0;
}