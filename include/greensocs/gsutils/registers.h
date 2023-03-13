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
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <scp/report.h>
#include "cciutils.h"
#include "rapidjson/document.h"

namespace gs {

#define GS_REGS_TOP "gs_regs."
class cci_tlm_object_if : public virtual cci::cci_param_if,
                          public virtual tlm::tlm_blocking_transport_if<>,
                          public virtual sc_core::sc_object
{
};

class gs_register_if : public virtual cci_tlm_object_if
{
public:
    virtual uint64_t get_offset() const = 0;
    virtual uint64_t get_size() const = 0;
    virtual bool get_read_only() const = 0;
    virtual const char* get_cci_name() const = 0;
    virtual const char* get_object_name() const = 0;

    virtual tlm::tlm_generic_payload* get_current_txn() const = 0;

    friend class cci::cci_value_converter<gs_register_if>;

    virtual cci::cci_param_untyped_handle create_param_handle(
        const cci::cci_originator& originator = cci::cci_originator()) const = 0;
};
template <class TYPE = uint32_t>
class gs_register : public virtual gs_register_if
{
    SCP_LOGGER(());

    // we could have inherited from this, but the diamond inheritance does not work due
    // to a missing virtual in cci_param_untyped.h
    cci::cci_param_typed<TYPE> value;

    uint64_t offset;
    bool read_only;
    uint64_t size;

    tlm::tlm_generic_payload* current_txn = nullptr;

public:
    gs_register(const char* _name, uint64_t _offset, bool _read_only, TYPE _default_val)
        : sc_core::sc_object(_name)
        , value(GS_REGS_TOP + std::string(_name), _default_val)
        , offset(_offset)
        , read_only(_read_only)
        , size(sizeof(TYPE)) {
        SCP_TRACE(())("Register construction {}");
    }
    /**
     * @brief helper functions
     */
    const char* get_object_name() const { return this->sc_core::sc_object::name(); }
    const char* get_cci_name() const { return value.name(); }
    uint64_t get_offset() const { return offset; }
    uint64_t get_size() const { return size; }
    bool get_read_only() const { return read_only; }

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
        SCP_DEBUG(()) << "Accessing " << get_cci_name();
        current_txn = &txn;
        switch (txn.get_command()) {
        case tlm::TLM_READ_COMMAND: {
            txn.set_response_status(tlm::TLM_OK_RESPONSE);
            TYPE data = value.get_value();
            if (txn.get_response_status() == tlm::TLM_OK_RESPONSE) {
                memcpy(ptr, &data, len);
            }
            break;
        }
        case tlm::TLM_WRITE_COMMAND:
            if (read_only) {
                txn.set_response_status(tlm::TLM_OK_RESPONSE);
                TYPE data;
                memcpy(&data, ptr, len);
                value.set_value(data);
            }
            break;

        default:
            SCP_FATAL(()) << "Unknown TLM command not supported\n";
            break;
        }
        current_txn = nullptr;
    }

    /**
     * @brief Get the current txn object
     * May be null if the access is not with a txn
     * @return tlm::tlm_generic_payload*
     */
    tlm::tlm_generic_payload* get_current_txn() const { return current_txn; }

    /* REPLICATE as much as the cci_param_if as possible. By adding a 'virtual' in
       cci_param_untyped.h class cci_param_untyped : public virtual cci_param_if then c++ seems to
       resolve the diamond inheritance, and we can inherit from cci_param_typed<TYPE> and get the
       desired functionality
    */
public:
    cci::cci_param_untyped_handle create_param_handle(const cci::cci_originator& originator) const {
        return value.create_param_handle();
    }
    bool unregister_pre_write_callback(const cci::cci_callback_untyped_handle& cb,
                                       const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return false;
    }
    bool unregister_post_write_callback(const cci::cci_callback_untyped_handle& cb,
                                        const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return false;
    }
    bool unregister_pre_read_callback(const cci::cci_callback_untyped_handle& cb,
                                      const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return false;
    }
    bool unregister_post_read_callback(const cci::cci_callback_untyped_handle& cb,
                                       const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return false;
    }

    cci::cci_callback_untyped_handle register_pre_write_callback(
        const cci::cci_callback_untyped_handle& cb, const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return cci::cci_callback_untyped_handle();
    }
    cci::cci_callback_untyped_handle register_post_write_callback(
        const cci::cci_callback_untyped_handle& cb, const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return cci::cci_callback_untyped_handle();
    }
    cci::cci_callback_untyped_handle register_pre_read_callback(
        const cci::cci_callback_untyped_handle& cb, const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return cci::cci_callback_untyped_handle();
    }
    cci::cci_callback_untyped_handle register_post_read_callback(
        const cci::cci_callback_untyped_handle& cb, const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return cci::cci_callback_untyped_handle();
    }
    bool unregister_all_callbacks(const cci::cci_originator& orig) {
        SCP_FATAL("Use param handle");
        return false;
    }
    bool has_callbacks() const { return value.has_callbacks(); }

    void set_cci_value(const cci::cci_value& val, const void* pwd,
                       const cci::cci_originator& originator) {
        value.set_cci_value(val, pwd, originator);
    }
    cci::cci_value get_cci_value(const cci::cci_originator& originator) const {
        return value.get_cci_value(originator);
    }
    cci::cci_value get_default_cci_value() const { return value.get_default_cci_value(); }
    std::string get_description() const { return value.get_description(); }
    void set_description(const std::string& desc) { value.set_description(desc); }
    void add_metadata(const std::string& name, const cci::cci_value& val,
                      const std::string& desc = "") {
        value.add_metadata(name, val, desc);
    }
    cci::cci_value_map get_metadata() const { return value.get_metadata(); }
    bool is_default_value() const { return value.is_default_value(); };
    bool is_preset_value() const { return value.is_preset_value(); }
    cci::cci_originator get_originator() const { return value.get_originator(); }
    cci::cci_originator get_value_origin() const { return value.get_value_origin(); }
    bool lock(const void* pwd = NULL) { return value.lock(pwd); }
    bool unlock(const void* pwd = NULL) { return value.unlock(pwd); }
    bool is_locked() const { return value.is_locked(); }
    const char* name() const { return value.name(); }
    cci::cci_param_mutable_type get_mutable_type() const { return value.get_mutable_type(); }
    const std::type_info& get_type_info() const { return value.get_type_info(); }
    cci::cci_param_data_category get_data_category() const { return value.get_data_category(); }
    bool reset() { return value.reset(); }

    // protected:
    void destroy(cci::cci_broker_handle broker) { value.destroy(broker); }
    void init(cci::cci_broker_handle broker_handle) { value.init(broker_handle); }
    template <typename T>
    const T& get_typed_value(const cci::cci_param_if& rhs) const {
        return value.get_typed_value(rhs);
    }

    // private:
    //  these functions should never be called, the implementation chosen for the functions that
    //  cause these to be called would be chosen down one path of the diamond inheritance
    void preset_cci_value(const cci::cci_value& preset, const cci::cci_originator& originator) {
        SCP_FATAL("Call to super class implementation of preset_cci_value");
    }
    void invalidate_all_param_handles() {
        SCP_FATAL("Call to super class implementation of invalidate_all_param_handles");
    }
    void set_raw_value(const void* vp, const void* pwd, const cci::cci_originator& originator) {
        SCP_FATAL("Call to super class implementation of set_raw_value");
    }
    const void* get_raw_value(const cci::cci_originator& originator) const {
        SCP_FATAL("Call to super class implementation of get_raw_value");
        return nullptr;
    }
    const void* get_raw_default_value() const {
        SCP_FATAL("Call to super class implementation of get_raw_default_value");
        return nullptr;
    }

    void add_param_handle(cci::cci_param_untyped_handle* param_handle) {
        value.add_param_handle(param_handle);
    }
    void remove_param_handle(cci::cci_param_untyped_handle* param_handle) {
        value.remove_param_handle(param_handle);
    }

    const TYPE& get_value() const { return get_value(get_originator()); }
    const TYPE& get_value(const cci::cci_originator& originator) const {
        return value.get_value(get_originator());
    };
    void set_value(const TYPE& val) { set_value(val, NULL); }
    void set_value(const TYPE& val, const void* pwd) { value.set_value(val, pwd); }
};

/**
 * @brief Helper function to find a register by fully qualified name
 * @param m     current parent object (use null to start from the top)
 * @param name  name being searched for
 * @return sc_core::sc_object* return object if found
 */
static struct gs::gs_register_if* find_register(std::string name, sc_core::sc_object* m = nullptr) {
    auto reg = dynamic_cast<gs::gs_register_if*>(m);
    if (reg && (name == m->name() || name == std::string(reg->get_cci_name()))) {
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
    SCP_LOGGER(());

private:
    sc_core::sc_vector<gs::gs_register_if> reg_info;

    std::multimap<uint64_t, gs::gs_register_if*> regs_map;

public:
    tlm_utils::multi_passthrough_target_socket<reg_bank> target_socket;

    struct reg_bank_info_s {
        std::string name;
        uint64_t offset;
        bool read_only;
        uint64_t default_val;
        int bitwidth;
    };
    reg_bank(sc_core::sc_module_name _name, std::vector<reg_bank_info_s> _reg_info)
        : sc_core::sc_module(_name), target_socket("target_socket") {
        SCP_DEBUG(())("Reg bank Constructor");
        target_socket.register_b_transport(this, &reg_bank::b_transport);

        reg_info.init(
            _reg_info.size(), [this, _reg_info](const char* name, size_t n) -> gs::gs_register_if* {
                switch (_reg_info[n].bitwidth) {
                case 64:
                    return new gs_register<uint64_t>(_reg_info[n].name.c_str(), _reg_info[n].offset,
                                                     _reg_info[n].read_only,
                                                     _reg_info[n].default_val);
                case 32:
                    return new gs_register<uint32_t>(_reg_info[n].name.c_str(), _reg_info[n].offset,
                                                     _reg_info[n].read_only,
                                                     _reg_info[n].default_val);
                case 16:
                    return new gs_register<uint16_t>(_reg_info[n].name.c_str(), _reg_info[n].offset,
                                                     _reg_info[n].read_only,
                                                     _reg_info[n].default_val);
                case 8:
                    return new gs_register<uint8_t>(_reg_info[n].name.c_str(), _reg_info[n].offset,
                                                    _reg_info[n].read_only,
                                                    _reg_info[n].default_val);
                default:
                    SCP_FATAL(())("Can only handle 64,32,16,8 bitwidths");
                    return nullptr;
                }
            });
        for (auto r = reg_info.begin(); r != reg_info.end(); r++) {
            regs_map.emplace(std::make_pair((*r).get_offset(), &(*r)));
        }
        reset();
    }

    void b_transport(int id, tlm::tlm_generic_payload& txn, sc_core::sc_time& delay) {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();

        auto range = regs_map.equal_range(addr);
        auto f = regs_map.lower_bound(addr);
        auto l = regs_map.upper_bound(addr + len - 1);
        if (f == regs_map.end() && l == regs_map.end()) {
            SCP_WARN(())("Attempt to access unknown register at offset 0x{:x}", addr);
            txn.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }
        for (auto ireg = f; ireg != l; ireg++) {
            gs::gs_register_if* reg = static_cast<gs::gs_register_if*>(ireg->second);

            txn.set_data_length(reg->get_size());
            txn.set_data_ptr(ptr + (reg->get_offset() - addr));
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
};

} // namespace gs

#endif // GS_REGISTERS_H