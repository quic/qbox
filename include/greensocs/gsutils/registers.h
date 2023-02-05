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

#ifndef GS_REGISTERS_H
#define GS_REGISTERS_H

#include <systemc>
#include <cci_configuration>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <scp/report.h>
#include "cciutils.h"

// Move to SCP ?

namespace gs {
// Move to SCP ?

#define GS_REGS_TOP "gs_regs."
class cci_tlm_object_if : public virtual cci::cci_param_if,
                          public virtual tlm::tlm_blocking_transport_if<>,
                          public virtual sc_core::sc_object
{
};

class gs_register_if : public virtual cci_tlm_object_if
{
public:
    virtual uint64_t get_address() = 0;
    virtual uint64_t get_size() = 0;
    virtual bool get_read_only() = 0;
    virtual const char* cci_name() = 0;
    virtual const char* name() = 0;
};
template <class TYPE = uint32_t>
class gs_register : public virtual gs_register_if, public virtual cci::cci_param_typed<TYPE>
{
    SCP_LOGGER(());

    uint64_t address;
    bool read_only;
    uint64_t size;

public:
    gs_register(const char* _name, uint64_t _address, bool _read_only, TYPE _default_val)
        : sc_core::sc_object(_name)
        , cci::cci_param_typed<TYPE>(GS_REGS_TOP + std::string(_name), _default_val)
        , address(_address)
        , read_only(_read_only)
        , size(sizeof(TYPE)) {}
    /**
     * @brief helper functions
     */
    const char* name() { return this->sc_core::sc_object::name(); }
    const char* cci_name() { return this->cci::cci_param_typed<TYPE>::name(); }
    uint64_t get_address() { return address; }
    uint64_t get_size() { return size; }
    bool get_read_only() { return read_only; }

    /**
     * @brief Perform b_transport on gs_register
     *
     * @param txn
     * @param delay
     */
    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay) {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        if (len != size) {
            SCP_FATAL(())("Register size does not match request\n");
        }
        SCP_DEBUG(()) << "Accessing " << name();
        switch (txn.get_command()) {
        case tlm::TLM_READ_COMMAND: {
            TYPE data = this->get_value();
            memcpy(ptr, &data, len);
            txn.set_response_status(tlm::TLM_OK_RESPONSE);
            break;
        }
        case tlm::TLM_WRITE_COMMAND:
            if (read_only) {
                uint32_t data;
                memcpy(&data, ptr, len);
                this->set_value(data);
                txn.set_response_status(tlm::TLM_OK_RESPONSE);
            }
            break;

        default:
            SCP_FATAL(()) << "Unknown TLM command not supported\n";
            break;
        }
    }
};

/**
 * @brief Helper function to find a register by fully qualified name
 * @param m     current parent object (use null to start from the top)
 * @param name  name being searched for
 * @return sc_core::sc_object* return object if found
 */
static struct gs::gs_register_if* find_register(std::string name, sc_core::sc_object* m = nullptr) {
    auto reg = dynamic_cast<gs::gs_register_if*>(m);
    if (reg && (name == m->name() || name == std::string(reg->cci_name()))) {
        return dynamic_cast<gs::gs_register_if*>(m);
    }

    std::vector<sc_core::sc_object*> children;
    if (m) {
        children = m->get_child_objects();
    } else {
        children = sc_core::sc_get_top_level_objects();
    }

    auto ip_it = std::find_if(begin(children), end(children), [&name](sc_core::sc_object* o) {
        return (name.compare(0, strlen(o->name()), o->name()) == 0) && find_register(name, o);
    });
    if (ip_it != end(children)) {
        return find_register(name, *ip_it);
    } else {
        for (auto c : children) {
            auto ret = find_register(name, c);
            if (ret)
                return ret;
        }
    }
    return nullptr;
}

static struct std::vector<gs::gs_register_if*> all_registers(sc_core::sc_object* m = nullptr) {
    std::vector<gs::gs_register_if*> res;

    if (m && dynamic_cast<gs::gs_register_if*>(m)) {
        res.push_back(dynamic_cast<gs::gs_register_if*>(m));
    }

    std::vector<sc_core::sc_object*> children;
    if (m) {
        children = m->get_child_objects();
    } else {
        children = sc_core::sc_get_top_level_objects();
    }
    for (auto c : children) {
        std::vector<gs::gs_register_if*> cres = all_registers(c);
        res.insert(res.end(), cres.begin(), cres.end());
    }
    return res;
}

class reg_bank : public sc_core::sc_module
{
private:
    sc_core::sc_vector<gs::gs_register_if> reg_info;

    std::multimap<uint64_t, gs::gs_register_if*> regs_map;

public:
    tlm_utils::simple_target_socket<reg_bank> target_socket;

    struct reg_bank_info_s {
        const char* name;
        uint64_t address;
        bool read_only;
        uint64_t default_val;
        uint64_t mask;
        int bitwidth;
    };
    reg_bank(sc_core::sc_module_name _name, std::vector<reg_bank_info_s> _reg_info)
        : sc_core::sc_module(_name), target_socket("target_socket") {
        SCP_DEBUG(())("Constructor");
        target_socket.register_b_transport(this, &reg_bank::b_transport);

        reg_info.init(
            _reg_info.size(), [this, _reg_info](const char* name, size_t n) -> gs::gs_register_if* {
                switch (_reg_info[n].bitwidth) {
                case 64:
                    return new gs_register<uint64_t>(_reg_info[n].name, _reg_info[n].address,
                                                         _reg_info[n].read_only,
                                                         _reg_info[n].default_val);
                case 32:
                    return new gs_register<uint32_t>(_reg_info[n].name, _reg_info[n].address,
                                                         _reg_info[n].read_only,
                                                         _reg_info[n].default_val);
                case 16:
                    return new gs_register<uint16_t>(_reg_info[n].name, _reg_info[n].address,
                                                         _reg_info[n].read_only,
                                                         _reg_info[n].default_val);
                case 8:
                    return new gs_register<uint8_t>(_reg_info[n].name, _reg_info[n].address,
                                                        _reg_info[n].read_only,
                                                        _reg_info[n].default_val);
                default:
                    SCP_FATAL(())("Can only handle 64,32,16,8 bitwidths");
                    return nullptr;
                }
            });
        for (auto r = reg_info.begin(); r != reg_info.end(); r++) {
            regs_map.emplace(std::make_pair((*r).get_address(), &(*r)));
        }
        reset();
    }

    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay) {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();

        auto range = regs_map.equal_range(addr);
        auto f = regs_map.lower_bound(addr);
        auto l = regs_map.upper_bound(addr + len - 1);
        if (f == regs_map.end() && l == regs_map.end()) {
            SCP_WARN(())("Attempt to access unknown register at address 0x{:x}", addr);
            txn.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }
        for (auto ireg = f; ireg != l; ireg++) {
            gs::gs_register_if* reg = static_cast<gs::gs_register_if*>(ireg->second);

            txn.set_data_length(reg->get_size());
            txn.set_data_ptr(ptr + (reg->get_address() - addr));
            reg->b_transport(txn, delay);
        }
        txn.set_data_length(len);
        txn.set_data_ptr(ptr);
    }

    void reset() {
        for (auto r = reg_info.begin(); r != reg_info.end(); r++) {
            (*r).reset();
        }
    }

    SCP_LOGGER(());
};

} // namespace gs

#endif // GS_REGISTERS_H