/*
 * This file is part of libqbox
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_LIBQEMU_PLUGIN_H
#define _LIBQBOX_COMPONENTS_LIBQEMU_PLUGIN_H
#include <systemc>

#include <cciutils.h>
#include <libqemu-cxx/libqemu-cxx.h>

#include <scp/report.h>
#include <cci_configuration>

#include <unordered_map>

#define REMOVE_PARENS(...)      REMOVE_PARENS_IMPL __VA_ARGS__
#define REMOVE_PARENS_IMPL(...) __VA_ARGS__

/**
 * Macro to register a callback function with QEMU.
 *
 * This macro bridges the gap between QEMU's C code and QBox's C++ code.
 * QEMU's plugin API requires static functions in C. To handle this, we use a
 * static instance pointer (self) to call the appropriate member function
 * of the LibQemuPlugin class.
 *
 * This macro allows us to define the callback in a way that integrates seamlessly
 * with QBox's C++ code, enabling the registration and usage of the callback functions
 * as required by QEMU.
 *
 * The macro constructs the QEMU plugin registration function name using the provided
 * callback name, qemu_plugin_register_##cb_name##_cb.
 *
 * @param cb_name The callback name to be used.
 * @param bridge_cb_param_types Types of parameters for the bridge cb function.
 * @param bridge_cb_params Parameters for the bridge cb function.
 * @param reg_params_types Types of parameters for the registration function.
 * @param reg_params Parameters for the registration function.
 */
#define REGISTER_CALLBACK_ID(cb_name, bridge_cb_param_types, bridge_cb_params, reg_params_types, reg_params) \
    virtual void cb_name(qemu_plugin_id_t id, REMOVE_PARENS(bridge_cb_param_types)) {}                       \
    static void bridge_##cb_name(qemu_plugin_id_t id, REMOVE_PARENS(bridge_cb_param_types))                  \
    {                                                                                                        \
        LibQemuPlugin* self = LibQemuPlugin::get_instance(id);                                               \
        assert(self);                                                                                        \
        self->cb_name(id, REMOVE_PARENS(bridge_cb_params));                                                  \
    }                                                                                                        \
    void enable_##cb_name reg_params_types { m_inst.p_api.qemu_plugin_register_##cb_name##_cb reg_params; }

#define REGISTER_CALLBACK_USERDATA(cb_name, bridge_cb_param_types, bridge_cb_params, reg_params_types, reg_params) \
    virtual void cb_name(REMOVE_PARENS(bridge_cb_param_types), void* userdata) {}                                  \
    static void bridge_##cb_name(REMOVE_PARENS(bridge_cb_param_types), void* userdata)                             \
    {                                                                                                              \
        LibQemuPlugin* self = reinterpret_cast<LibQemuPlugin*>(userdata);                                          \
        assert(self);                                                                                              \
        self->cb_name(REMOVE_PARENS(bridge_cb_params), userdata);                                                  \
    }                                                                                                              \
    void enable_##cb_name reg_params_types { m_inst.p_api.qemu_plugin_register_##cb_name##_cb reg_params; }
class LibQemuPlugin : public sc_core::sc_module
{
    SCP_LOGGER();

protected:
    cci::cci_param<uint64_t> id;
    qemu::LibQemu& m_inst;
    static std::unordered_map<qemu_plugin_id_t, LibQemuPlugin*> id_map;

public:
    LibQemuPlugin(const sc_core::sc_module_name& nm, qemu::LibQemu& inst)
        : sc_module(nm), m_inst(inst), id("id", 0, "qemu plugin id")
    {
    }

    static LibQemuPlugin* get_instance(qemu_plugin_id_t id)
    {
        auto it = id_map.find(id);
        return it != id_map.end() ? it->second : nullptr;
    }

    void push_plugin_args(std::string plugin_path)
    { /*
       * Pushing the plugin argument here instead of in the constructor
       * because we need to wait for QemuInstance to push libqbox arguments first.
       */
        SCP_DEBUG(())("push_plugin_args, key: {} ", id.name());
        SCP_DEBUG(())("push_plugin_args, plugin_path: {} ", plugin_path);
        std::stringstream opts;
        opts << plugin_path;
        opts << ",key=" << id.name();
        m_inst.push_qemu_arg("-plugin");
        m_inst.push_qemu_arg(opts.str().c_str());
    }

    void end_of_elaboration() override
    {
        SCP_DEBUG(())("LibQemuPlugin: key {}, id: {}", id.name(), id);
        id_map[id] = this;
    }
    REGISTER_CALLBACK_ID(vcpu_init, (unsigned int vcpu_index), (vcpu_index),
                         (qemu_plugin_id_t id, qemu_plugin_vcpu_simple_cb_t cb), (id, cb))
    REGISTER_CALLBACK_ID(vcpu_exit, (unsigned int vcpu_index), (vcpu_index),
                         (qemu_plugin_id_t id, qemu_plugin_vcpu_simple_cb_t cb), (id, cb))
    REGISTER_CALLBACK_ID(vcpu_idle, (unsigned int vcpu_index), (vcpu_index),
                         (qemu_plugin_id_t id, qemu_plugin_vcpu_simple_cb_t cb), (id, cb))
    REGISTER_CALLBACK_ID(vcpu_resume, (unsigned int vcpu_index), (vcpu_index),
                         (qemu_plugin_id_t id, qemu_plugin_vcpu_simple_cb_t cb), (id, cb))
    REGISTER_CALLBACK_ID(vcpu_tb_trans, (qemu_plugin_tb * tb), (tb),
                         (qemu_plugin_id_t id, qemu_plugin_vcpu_tb_trans_cb_t cb), (id, cb))
    REGISTER_CALLBACK_USERDATA(vcpu_tb_exec, (unsigned int vcpu_index), (vcpu_index),
                               (qemu_plugin_tb * tb, qemu_plugin_vcpu_udata_cb_t cb, qemu_plugin_cb_flags flags,
                                void* userdata),
                               (tb, cb, flags, userdata))
    REGISTER_CALLBACK_USERDATA(vcpu_tb_exec_cond, (unsigned int vcpu_index), (vcpu_index),
                               (qemu_plugin_tb * tb, qemu_plugin_vcpu_udata_cb_t cb, qemu_plugin_cb_flags flags,
                                qemu_plugin_cond cond, qemu_plugin_u64 entry, uint64_t imm, void* userdata),
                               (tb, cb, flags, cond, entry, imm, userdata))
    REGISTER_CALLBACK_USERDATA(vcpu_insn_exec, (unsigned int vcpu_index), (vcpu_index),
                               (qemu_plugin_insn * insn, qemu_plugin_vcpu_udata_cb_t cb, qemu_plugin_cb_flags flags,
                                void* userdata),
                               (insn, cb, flags, userdata))
    REGISTER_CALLBACK_USERDATA(vcpu_insn_exec_cond, (unsigned int vcpu_index), (vcpu_index),
                               (qemu_plugin_insn * insn, qemu_plugin_vcpu_udata_cb_t cb, qemu_plugin_cb_flags flags,
                                qemu_plugin_cond cond, qemu_plugin_u64 entry, uint64_t imm, void* userdata),
                               (insn, cb, flags, cond, entry, imm, userdata))
    REGISTER_CALLBACK_USERDATA(vcpu_mem, (unsigned int vcpu_index, qemu_plugin_meminfo_t info, uint64_t vaddr),
                               (vcpu_index, info, vaddr),
                               (qemu_plugin_insn * insn, qemu_plugin_vcpu_mem_cb_t cb, qemu_plugin_cb_flags flags,
                                qemu_plugin_mem_rw rw, void* userdata),
                               (insn, cb, flags, rw, userdata))
    REGISTER_CALLBACK_ID(vcpu_syscall,
                         (unsigned int vcpu_index, int64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
                          uint64_t a5, uint64_t a6, uint64_t a7, uint64_t a8),
                         (vcpu_index, num, a1, a2, a3, a4, a5, a6, a7, a8),
                         (qemu_plugin_id_t id, qemu_plugin_vcpu_syscall_cb_t cb), (id, cb))
    REGISTER_CALLBACK_ID(vcpu_syscall_ret, (unsigned int vcpu_idx, int64_t num, int64_t ret), (vcpu_idx, num, ret),
                         (qemu_plugin_id_t id, qemu_plugin_vcpu_syscall_ret_cb_t cb), (id, cb))

    // qemu_plugin_register_flush_cb /* does not follow the same pattern and is currently not being used */
    // qemu_plugin_register_atexit_cb /* cannot be used due to a conflict with qbox atexit */
};
std::unordered_map<qemu_plugin_id_t, LibQemuPlugin*> LibQemuPlugin::id_map;
#endif //_LIBQBOX_COMPONENTS_LIBQEMU_PLUGIN_H