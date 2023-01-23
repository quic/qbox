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

#include <systemc>
#include <cci_configuration>
#include <tlm>

// Move to SCP ?

namespace gs {
// Move to SCP ?
template <class TYPE = uint32_t>
struct reg_info_s : sc_core::sc_object, cci::cci_param<TYPE> {
    uint64_t address;
    bool read_only;
    uint64_t size;
    SCP_LOGGER(());

    const char* name() { return this->sc_core::sc_object::name(); }
    reg_info_s(const char* _name, uint64_t _address, bool _read_only, uint32_t _default_val)
        : sc_core::sc_object(_name)
        , address(_address)
        , read_only(_read_only)
        , cci::cci_param<TYPE>(std::string(_name) + "_value", _default_val)
        , size(sizeof(TYPE)) {}

    /**
     * @brief Perform b_transport on register
     *
     * @param txn
     * @param delay
     */
    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay) {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();

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
template <class TYPE = uint32_t>
static struct gs::reg_info_s<TYPE>* find_register(std::string name,
                                                  sc_core::sc_object* m = nullptr) {
    if (m && name == m->name()) {
        return dynamic_cast<gs::reg_info_s<TYPE>*>(m);
    }
    if (name.find("_value") == name.length() - strlen("_value")) {
        name = name.substr(0, name.find("_value"));
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
    }
    return nullptr;
}

template <class TYPE = uint32_t>
static struct std::vector<gs::reg_info_s<TYPE>*> all_registers(sc_core::sc_object* m = nullptr) {
    std::vector<gs::reg_info_s<TYPE>*> res;

    if (m && dynamic_cast<gs::reg_info_s<TYPE>*>(m)) {
        res.push_back(dynamic_cast<gs::reg_info_s<TYPE>*>(m));
    }

    std::vector<sc_core::sc_object*> children;
    if (m) {
        children = m->get_child_objects();
    } else {
        children = sc_core::sc_get_top_level_objects();
    }
    for (auto c : children) {
        std::vector<gs::reg_info_s<TYPE>*> cres = all_registers(c);
        res.insert(res.end(), cres.begin(), cres.end());
    }
    return res;
}
} // namespace gs