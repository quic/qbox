/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GREENSOCS_MODULE_FACTORY_H
#define GREENSOCS_MODULE_FACTORY_H

#define SC_ALLOW_DEPRECATED_IEEE_API

#include "cciutils.h"
#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <dlfcn.h>

#include <scp/report.h>
#include <scp/helpers.h>

#include <libgsutils.h>
#include <libgssync.h>

#include <ports/target-signal-socket.h>
#include <ports/initiator-signal-socket.h>

#ifndef WITHOUT_QEMU
#include <ports/target.h>
#include <ports/initiator.h>
#include <ports/qemu-target-signal-socket.h>
#include <ports/qemu-initiator-signal-socket.h>
#endif

#include <ports/biflow-socket.h>
#include <module_factory_registery.h>
#include <transaction_forwarder_if.h>
#include <tlm_sockets_buswidth.h>
#include <algorithm>
#include <vector>
#include <iterator>
#include <regex>
#include <cctype>

namespace gs {
namespace ModuleFactory {

class ContainerBase : public virtual sc_core::sc_module, public transaction_forwarder_if<CONTAINER>
{
    /**
     * @brief construct a module using the pre-register CCI functor, with typed arguments from a CCI
     * value list. The functor is expected to call new.
     *
     * @param moduletype The name of the type of module to be constructed
     * @param name The name to be given to this instance of the module
     * @param args The CCI list of arguments passed to the constructor (which will be type punned)
     * @return sc_core::sc_module* The module constructed.
     */

public:
    SCP_LOGGER(());

    sc_core::sc_module* construct_module(std::string moduletype, sc_core::sc_module_name name, cci::cci_value_list args)
    {
        gs::cci_constructor_vl m_fac;
        cci::cci_value v = cci::cci_value::from_json("\"" + moduletype + "\"");
        if (!v.try_get<gs::cci_constructor_vl>(m_fac)) {
            register_module_from_dylib(name);
        }

        if (v.try_get<gs::cci_constructor_vl>(m_fac)) {
            /**
             * (m_fac)(name, args) will return raw pointer of unmanaged dynamically allocated moduletype:
             *  new moduletype(name, args)
             */
            sc_core::sc_module* mod = (m_fac)(name, args);
            if (mod) {
                m_constructedModules.emplace_back(mod);
                return mod;
            } else {
                SC_REPORT_ERROR("ModuleFactory", ("Null module type: " + moduletype).c_str());
            }
        } else {
            SC_REPORT_ERROR("ModuleFactory", ("Can't find module type: " + moduletype).c_str());
        }
        __builtin_unreachable();
    }

    /**
     * @brief Helper to construct a module that takes no arguments.
     *
     */
    sc_core::sc_module* construct_module(std::string moduletype, sc_core::sc_module_name name)
    {
        cci::cci_value_list emptyargs;
        return construct_module(moduletype, name, emptyargs);
    }

    std::string get_parent_name(const std::string& module_name)
    {
        return module_name.substr(0, module_name.find_last_of("."));
    }

    bool is_container_arg_mod_name(const std::string& name)
    {
        if (!this->container_mod_arg) return false;
        std::string arg_mod_full_name_str = std::string(this->container_mod_arg->name());
        std::string arg_mod_name_str = arg_mod_full_name_str.substr(get_parent_name(arg_mod_full_name_str).size() + 1);
        return name == arg_mod_name_str;
    }

    /**
     * @brief Get the module args helper to find argument list for a module constructor
     *
     * @param modulename name of the module from which to get the .arg[...] list
     * @return cci::cci_value_list
     */
    cci::cci_value_list get_module_args(std::string modulename)
    {
        cci::cci_value_list args;
        for (int i = 0; cci::cci_get_broker().has_preset_value(modulename + ".args." + std::to_string(i)); i++) {
            SCP_TRACE(())("Looking for : {}.args.{}", modulename, std::to_string(i));

            gs::cci_clear_unused(m_broker, modulename + ".args." + std::to_string(i));
            auto arg = cci::cci_get_broker().get_preset_cci_value(modulename + ".args." + std::to_string(i));
            if (arg.is_string() && std::string(arg.get_string())[0] == '&') {
                std::string parent = is_container_arg_mod_name(std::string(arg.get_string()).erase(0, 1))
                                         ? get_parent_name(std::string(this->container_mod_arg->name()))
                                         : get_parent_name(modulename);
                if (std::string(arg.get_string()).find(parent, 1) == std::string::npos)
                    arg.set_string("&" + parent + "." + std::string(arg.get_string()).erase(0, 1));
            }
            args.push_back(arg);
        }
        return args;
    }

    template <typename I, typename T>
    bool try_bind(sc_core::sc_object* i_obj, sc_core::sc_object* t_obj)
    {
        auto i = dynamic_cast<I*>(i_obj);
        auto t = dynamic_cast<T*>(t_obj);
        if (i && t) {
            i->bind(*t);
            SCP_INFO(())
            ("Binding initiator socket: {}({}) with the target socket: {}({})", i_obj->name(), typeid(i).name(),
             t_obj->name(), typeid(t).name());
            return true;
        }
        t = dynamic_cast<T*>(i_obj);
        i = dynamic_cast<I*>(t_obj);
        if (i && t) {
            i->bind(*t);
            SCP_INFO(())
            ("Binding initiator socket: {}({}) with the target socket: {}({})", t_obj->name(), typeid(i).name(),
             i_obj->name(), typeid(t).name());
            return true;
        }
        return false;
    }

    /* bind all know port types */
    /**
     * @brief Bind all known port types
     *
     * @param modulename The LOCAL name of the module to bind it's ports
     * @param type The type of the module
     * @param m The module itself.
     */
    void name_bind(sc_core::sc_module* m)
    {
        for (auto param : sc_cci_children(m->name())) {
            /* We should have a param like
             *  foo = &a.baa
             * foo should relate to an object in this model. a.b.bar should relate to an other
             * object Note that the names are relative to the current module (this).
             */
            std::string targetname = std::string(m->name()) + "." + param;
            gs::cci_clear_unused(m_broker, targetname + ".0");
            if (m_broker.get_preset_cci_value(targetname + ".0").is_string()) {
                // deal with an alias
                std::string src = m_broker.get_preset_cci_value(targetname + ".0").get_string();
                m_broker.set_preset_cci_value(targetname + ".address", m_broker.get_preset_cci_value(src + ".address"));
                m_broker.set_preset_cci_value(targetname + ".size", m_broker.get_preset_cci_value(src + ".size"));
            }
            std::string bindname = targetname + ".bind";
            gs::cci_clear_unused(m_broker, bindname);
            if (!m_broker.get_preset_cci_value(bindname).is_string()) {
                continue;
            }
            std::string initname = m_broker.get_preset_cci_value(bindname).get_string();

            if (initname.at(0) != '&') continue; // all port binds are donated with a '&', so not for us

            if (initname.find(';') != std::string::npos) {
                order_bind(m, initname, targetname);
            } else {
                initname.erase(0, 1); // remove leading '&'
                initname = std::string(name()) + "." + initname;
                SCP_TRACE(())("Bind {} to {}", targetname, initname);
                do_binding(m, initname, targetname);
            }
        }
    }

    /**
     * FIXME: There should be a better way of doing that.Trying to bind every possible
     * combination of socket types is not scalable nor readable solution.
     *
     */
    template <unsigned int BIND_BUSWIDTH>
    bool try_bind_all(sc_core::sc_object* i_obj, sc_core::sc_object* t_obj)
    {
        if ((try_bind<tlm_utils::multi_init_base<BIND_BUSWIDTH>, tlm::tlm_base_target_socket<BIND_BUSWIDTH>>(i_obj,
                                                                                                             t_obj)) ||
            (try_bind<tlm_utils::multi_init_base<BIND_BUSWIDTH>, tlm_utils::multi_target_base<BIND_BUSWIDTH>>(i_obj,
                                                                                                              t_obj)) ||
            (try_bind<tlm::tlm_base_initiator_socket<BIND_BUSWIDTH>, tlm::tlm_base_target_socket<BIND_BUSWIDTH>>(
                i_obj, t_obj)) ||
            (try_bind<tlm::tlm_base_initiator_socket<BIND_BUSWIDTH>, tlm_utils::multi_target_base<BIND_BUSWIDTH>>(
                i_obj, t_obj)) ||
            (try_bind<tlm::tlm_base_initiator_socket<BIND_BUSWIDTH, tlm::tlm_fw_transport_if<>,
                                                     tlm::tlm_bw_transport_if<>, 1, sc_core::SC_ZERO_OR_MORE_BOUND>,
                      tlm_utils::multi_target_base<BIND_BUSWIDTH>>(i_obj, t_obj)) ||
            (try_bind<gs::biflow_bindable, gs::biflow_bindable>(i_obj, t_obj)) ||
            (try_bind<tlm::tlm_base_initiator_socket<BIND_BUSWIDTH, tlm::tlm_fw_transport_if<>,
                                                     tlm::tlm_bw_transport_if<>, 1, sc_core::SC_ZERO_OR_MORE_BOUND>,
                      tlm::tlm_base_target_socket<BIND_BUSWIDTH>>(i_obj, t_obj)) ||
            (try_bind<
                tlm::tlm_initiator_socket<BIND_BUSWIDTH, tlm::tlm_base_protocol_types, 1,
                                          sc_core::SC_ONE_OR_MORE_BOUND>,
                tlm::tlm_target_socket<BIND_BUSWIDTH, tlm::tlm_base_protocol_types, 1, sc_core::SC_ZERO_OR_MORE_BOUND>>(
                i_obj, t_obj)) ||
            (try_bind<
                tlm_utils::multi_init_base<BIND_BUSWIDTH>,
                tlm::tlm_target_socket<BIND_BUSWIDTH, tlm::tlm_base_protocol_types, 1, sc_core::SC_ZERO_OR_MORE_BOUND>>(
                i_obj, t_obj)) ||
            (try_bind<
                tlm::tlm_base_initiator_socket<BIND_BUSWIDTH, tlm::tlm_fw_transport_if<>, tlm::tlm_bw_transport_if<>, 1,
                                               sc_core::SC_ZERO_OR_MORE_BOUND>,
                tlm::tlm_target_socket<BIND_BUSWIDTH, tlm::tlm_base_protocol_types, 1, sc_core::SC_ZERO_OR_MORE_BOUND>>(
                i_obj, t_obj)) ||
            (try_bind<sc_core::sc_port<sc_core::sc_signal_inout_if<bool>, 0, sc_core::SC_ZERO_OR_MORE_BOUND>,
                      TargetSignalSocket<bool>>(i_obj, t_obj)) ||
            (try_bind<InitiatorSignalSocket<bool>, TargetSignalSocket<bool>>(i_obj, t_obj))
#ifndef WITHOUT_QEMU
            ||
            (try_bind<QemuInitiatorSocket<BIND_BUSWIDTH>, tlm::tlm_base_target_socket<BIND_BUSWIDTH>>(i_obj, t_obj)) ||
            (try_bind<QemuInitiatorSocket<BIND_BUSWIDTH>, tlm_utils::multi_target_base<BIND_BUSWIDTH>>(i_obj, t_obj)) ||
            (try_bind<tlm_utils::multi_init_base<BIND_BUSWIDTH>, QemuTargetSocket<BIND_BUSWIDTH>>(i_obj, t_obj)) ||
            (try_bind<tlm::tlm_base_initiator_socket<BIND_BUSWIDTH>, QemuTargetSocket<BIND_BUSWIDTH>>(i_obj, t_obj)) ||
            (try_bind<QemuInitiatorSocket<BIND_BUSWIDTH>, QemuTargetSocket<BIND_BUSWIDTH>>(i_obj, t_obj)) ||
            (try_bind<QemuInitiatorSignalSocket, QemuTargetSignalSocket>(i_obj, t_obj)) ||
            (try_bind<QemuInitiatorSignalSocket, TargetSignalSocket<bool>>(i_obj, t_obj)) ||
            (try_bind<InitiatorSignalSocket<bool>, QemuTargetSignalSocket>(i_obj, t_obj))
#endif
        ) {
            return true;
        } else {
            return false;
        }
    }

    std::list<std::string> PriorityConstruct(void)
    {
        std::list<std::string> args;
        std::set<std::string> done;
        std::list<std::string> todo;
        std::string module_name = std::string(sc_module::name());

        todo = gs::sc_cci_children(name());

        while (todo.size() > 0) {
            int i = 0;
            for (auto it = todo.begin(); it != todo.end();) {
                int count_args = 0;
                std::string name = *it;
                cci::cci_value_list mod_args = get_module_args(module_name + "." + name);
                for (auto arg : mod_args) {
                    if (arg.is_string()) {
                        std::string a = arg.get_string();
                        if (a.substr(0, module_name.size() + 1) == "&" + module_name) {
                            if (done.count(a.substr(module_name.size() + 2)) == 0) {
                                count_args += 1;
                                break;
                            }
                        }
                    }
                }
                if (count_args == 0) {
                    done.insert(name);
                    it = todo.erase(it);
                    i++;
                    args.push_back(name);
                } else {
                    it++;
                }
            }
            if (i == 0) {
                // loop dans todo qui n'a pas été construct
                for (auto t : todo) {
                    SCP_WARN(()) << "Module name which is not constructable: " << t;
                }
                SCP_FATAL(()) << "Module Not constructable";
            }
        }
        return args;
    }

    void register_module_from_dylib(sc_core::sc_module_name name)
    {
        std::string libname;
#ifdef __APPLE__
        if (m_broker.has_preset_value(std::string(sc_module::name()) + "." + std::string(name) + ".moduletype")) {
            libname = m_broker
                          .get_preset_cci_value(std::string(sc_module::name()) + "." + std::string(name) +
                                                ".moduletype")
                          .get_string();
            libname += ".dylib";
        }
        if (m_broker.has_preset_value(std::string(sc_module::name()) + "." + std::string(name) + ".dylib_path")) {
            libname = m_broker
                          .get_preset_cci_value(std::string(sc_module::name()) + "." + std::string(name) +
                                                ".dylib_path")
                          .get_string();
            libname += ".dylib";
        }
#else
        if (m_broker.has_preset_value(std::string(sc_module::name()) + "." + std::string(name) + ".moduletype")) {
            libname = m_broker
                          .get_preset_cci_value(std::string(sc_module::name()) + "." + std::string(name) +
                                                ".moduletype")
                          .get_string();
            libname += ".so";
        }
        if (m_broker.has_preset_value(std::string(sc_module::name()) + "." + std::string(name) + ".dylib_path")) {
            libname = m_broker
                          .get_preset_cci_value(std::string(sc_module::name()) + "." + std::string(name) +
                                                ".dylib_path")
                          .get_string();
            libname += ".so";
        }
#endif
        std::cout << "libname =" << libname << std::endl;
        void* libraryHandle = dlopen(libname.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!libraryHandle) {
            SCP_FATAL(()) << "Impossible to load the library check the path in the lua file or if the library "
                             "exist in your system: "
                          << dlerror();
        }
        m_dls.insert(libraryHandle);
        void (*module_register)() = reinterpret_cast<void (*)()>(dlsym(libraryHandle, "module_register"));
        if (!module_register) {
            SCP_WARN(()) << "The method module_register has not been found in the class " << name;
        }
        module_register();
    }

    void ModulesConstruct(void)
    {
        for (auto name : PriorityConstruct()) {
            auto mod_type_name = std::string(sc_module::name()) + "." + name + ".moduletype";
            if (m_broker.has_preset_value(mod_type_name)) {
                if (m_broker.has_preset_value(std::string(sc_module::name()) + "." + name + ".dont_construct")) {
                    sc_core::sc_object* no_construct_mod_obj = nullptr;
                    std::string no_construct_mod_name = std::string(sc_module::name()) + "." + name;
                    no_construct_mod_obj = gs::find_sc_obj(nullptr, no_construct_mod_name);
                    auto no_construct_mod = dynamic_cast<sc_core::sc_module*>(no_construct_mod_obj);
                    if (!no_construct_mod) {
                        SCP_FATAL(()) << "The object " << std::string(sc_module::name()) + "." + name
                                      << " is not yet constructed";
                    }
                    m_allModules.push_back(no_construct_mod);
                } else {
                    gs::cci_clear_unused(m_broker, mod_type_name);
                    cci::cci_value cci_type = gs::cci_get(m_broker, mod_type_name);
                    if (cci_type.is_string()) {
                        std::string mod_type = cci_type.get_string();
                        SCP_INFO(()) << "Adding a " << mod_type << " with name "
                                     << std::string(sc_module::name()) + "." + name;
                        cci::cci_value_list mod_args = get_module_args(std::string(sc_module::name()) + "." + name);
                        SCP_INFO(()) << mod_args.size() << " arguments found for " << mod_type;
                        sc_core::sc_module* m = construct_module(mod_type, name.c_str(), mod_args);
                        if (!m) {
                            SC_REPORT_ERROR(
                                "ModuleFactory",
                                ("Can't automatically handle argument list for module: " + mod_type).c_str());
                        }

                        m_allModules.push_back(m);
                        if (!m_local_pass) {
                            m_local_pass = dynamic_cast<transaction_forwarder_if<PASS>*>(m);
                        }
                    } else {
                        SCP_WARN(()) << "The value of the parameter moduletype of the module " << name
                                     << " is not a string";
                    }
                }
            } // else it's some other config
        }
        // bind everything
        for (auto mod : m_allModules) {
            name_bind(mod);
        }
    }

    using tlm_initiator_socket_type = tlm_utils::simple_initiator_socket_b<
        ContainerBase, DEFAULT_TLM_BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_type = tlm_utils::simple_target_socket_tagged_b<
        ContainerBase, DEFAULT_TLM_BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;

private:
    bool m_defer_modules_construct;
    std::list<sc_core::sc_module*> m_allModules;
    std::list<std::shared_ptr<sc_core::sc_module>> m_constructedModules;

    std::set<void*> m_dls;

public:
    cci::cci_broker_handle m_broker;
    cci::cci_param<std::string> moduletype; // for consistency
    cci::cci_param<uint32_t> p_tlm_initiator_ports_num;
    cci::cci_param<uint32_t> p_tlm_target_ports_num;
    cci::cci_param<uint32_t> p_initiator_signals_num;
    cci::cci_param<uint32_t> p_target_signals_num;
    sc_core::sc_vector<tlm_initiator_socket_type> initiator_sockets;
    sc_core::sc_vector<tlm_target_socket_type> target_sockets;
    sc_core::sc_vector<InitiatorSignalSocket<bool>> initiator_signal_sockets;
    sc_core::sc_vector<TargetSignalSocket<bool>> target_signal_sockets;
    TargetSignalSocket<bool> container_self_reset;
    transaction_forwarder_if<PASS>* m_local_pass;

    std::vector<cci::cci_param<gs::cci_constructor_vl>*> registered_mods;
    sc_core::sc_object* container_mod_arg;

    virtual ~ContainerBase()
    {
        m_constructedModules.reverse();
        m_constructedModules.clear();
        for (auto l : m_dls) {
            dlclose(l);
        }
    }

    std::shared_ptr<sc_core::sc_module> find_module_by_name(const std::string& mod_name)
    {
        auto ret = std::find_if(m_constructedModules.begin(), m_constructedModules.end(),
                                [&](auto mod) { return (mod && std::string(mod->name()) == mod_name); });
        if (ret == m_constructedModules.end()) {
            return nullptr;
        }
        return *ret;
    }

    template <typename T>
    std::shared_ptr<sc_core::sc_module> find_module_by_cci_param(const std::string& cci_param_name, const T& cmp_value)
    {
        T ret_val;
        auto ret = std::find_if(m_constructedModules.begin(), m_constructedModules.end(), [&](auto mod) {
            return (mod && m_broker.has_preset_value(std::string(mod->name()) + "." + cci_param_name) &&
                    m_broker.get_preset_cci_value(std::string(mod->name()) + "." + cci_param_name)
                        .template try_get<T>(ret_val) &&
                    (ret_val == cmp_value));
        });
        if (ret == m_constructedModules.end()) {
            return nullptr;
        }
        return *ret;
    }

    /**
     * @brief construct a Container, and all modules within it, and perform binding
     *
     * @param _n name to give the container
     */

    ContainerBase(const sc_core::sc_module_name _n, bool defer_modules_construct,
                  sc_core::sc_object* p_container_mod_arg = nullptr)
        : m_defer_modules_construct(defer_modules_construct)
        , m_broker(cci::cci_get_broker())
        , moduletype("moduletype", "", "Module type for the TLM container, must be \"Container\"")
        , p_tlm_initiator_ports_num("tlm_initiator_ports_num", 0, "number of tlm initiator ports")
        , p_tlm_target_ports_num("tlm_target_ports_num", 0, "number of tlm target ports")
        , p_initiator_signals_num("initiator_signals_num", 0, "number of initiator signals")
        , p_target_signals_num("target_signals_num", 0, "number of target signals")
        , initiator_sockets("initiator_socket")
        , target_sockets("target_socket")
        , initiator_signal_sockets("initiator_signal_socket")
        , target_signal_sockets("target_signal_socket")
        , container_self_reset("container_self_reset")
        , m_local_pass(nullptr)
        , container_mod_arg(p_container_mod_arg)
    {
        SCP_DEBUG(()) << "ContainerBase Constructor";

        initiator_sockets.init(p_tlm_initiator_ports_num.get_value(),
                               [this](const char* n, int i) { return new tlm_initiator_socket_type(n); });
        target_sockets.init(p_tlm_target_ports_num.get_value(),
                            [this](const char* n, int i) { return new tlm_target_socket_type(n); });
        initiator_signal_sockets.init(p_initiator_signals_num.get_value(),
                                      [this](const char* n, int i) { return new InitiatorSignalSocket<bool>(n); });
        target_signal_sockets.init(p_target_signals_num.get_value(),
                                   [this](const char* n, int i) { return new TargetSignalSocket<bool>(n); });

        for (int i = 0; i < p_tlm_target_ports_num.get_value(); i++) {
            target_sockets[i].register_b_transport(this, &ContainerBase::b_transport, i);
            target_sockets[i].register_transport_dbg(this, &ContainerBase::transport_dbg, i);
            target_sockets[i].register_get_direct_mem_ptr(this, &ContainerBase::get_direct_mem_ptr, i);
        }

        for (int i = 0; i < p_tlm_initiator_ports_num.get_value(); i++) {
            initiator_sockets[i].register_invalidate_direct_mem_ptr(this, &ContainerBase::invalidate_direct_mem_ptr);
        }

        for (int i = 0; i < p_target_signals_num.get_value(); i++) {
            target_signal_sockets[i].register_value_changed_cb([&, i](bool value) {
                if (m_local_pass) m_local_pass->fw_handle_signal(i, value);
            });
        }

        container_self_reset.register_value_changed_cb([this](bool value) { do_reset(value); });

        auto mods = gs::ModuleFactory::GetAvailableModuleList();

        while (mods->size()) {
            registered_mods.push_back((mods->back())());
            mods->pop_back();
        }

        if (!m_defer_modules_construct) ModulesConstruct();
    }

    void fw_b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) override
    {
        SCP_DEBUG(()) << "calling b_transport on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
        initiator_sockets[id]->b_transport(trans, delay);
        SCP_DEBUG(()) << "return from b_transport on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
    }

    unsigned int fw_transport_dbg(int id, tlm::tlm_generic_payload& trans) override
    {
        SCP_DEBUG(()) << "calling transport_dbg on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
        unsigned int ret = initiator_sockets[id]->transport_dbg(trans);
        SCP_DEBUG(()) << "return from transport_dbg on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
        return ret;
    }

    bool fw_get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) override
    {
        SCP_DEBUG(()) << "calling get_direct_mem_ptr on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
        bool ret = initiator_sockets[id]->get_direct_mem_ptr(trans, dmi_data);
        SCP_DEBUG(()) << "return from get_direct_mem_ptr on initiator_socket_" << id << " "
                      << " RET: " << std::boolalpha << ret << " " << scp::scp_txn_tostring(trans)
                      << " IS_READ_ALLOWED: " << std::boolalpha << dmi_data.is_read_allowed() << " "
                      << " IS_WRITE_ALLOWED: " << std::boolalpha << dmi_data.is_write_allowed();
        return ret;
    }

    void fw_invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) override
    {
        SCP_DEBUG(()) << " " << name() << " invalidate_direct_mem_ptr "
                      << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;
        for (int i = 0; i < target_sockets.size(); i++) {
            target_sockets[i]->invalidate_direct_mem_ptr(start, end);
        }
    }

    void fw_handle_signal(int id, bool value) override
    {
        SCP_DEBUG(()) << "calling handle_signal on initiator_signal_socket_" << id << " value: " << std::boolalpha
                      << value;
        initiator_signal_sockets[id]->write(value);
    }

private:
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        if (m_local_pass) {
            m_local_pass->fw_b_transport(id, trans, delay);
        }
    }

    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        if (m_local_pass) {
            return m_local_pass->fw_transport_dbg(id, trans);
        }
        return 0;
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        if (m_local_pass) {
            return m_local_pass->fw_get_direct_mem_ptr(id, trans, dmi_data);
        }
        return false;
    }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        if (m_local_pass) {
            m_local_pass->fw_invalidate_direct_mem_ptr(start, end);
        }
    }

    std::vector<std::string> parse_order_str(const std::string& reset_order)
    {
        std::vector<std::string> ret;
        if (reset_order.empty()) {
            return ret;
        }
        std::string reset_order_tr = reset_order;
        reset_order_tr.erase(
            std::remove_if(reset_order_tr.begin(), reset_order_tr.end(), [](char c) { return std::isspace(c); }),
            reset_order_tr.end());
        size_t sig_count = std::count_if(reset_order_tr.begin(), reset_order_tr.end(), [](char c) { return c == ';'; });
        ret.resize(sig_count + 1);
        const std::regex separator(R"(;)");
        std::copy(std::sregex_token_iterator(reset_order_tr.begin(), reset_order_tr.end(), separator, -1),
                  std::sregex_token_iterator(), ret.begin());
        return ret;
    }

    void do_binding(sc_core::sc_module* module_obj, const std::string& initiator_name, const std::string& target_name)
    {
        sc_core::sc_object* i_obj = nullptr;
        sc_core::sc_object* t_obj = nullptr;
        /* find the initiator object */

        i_obj = find_sc_obj(nullptr, initiator_name);

        /* find the target object which must be inside this object */
        t_obj = find_sc_obj(module_obj, target_name);

        // actually you could probably just cast everything to (tlm::tlm_target_socket<>*), but
        // we will dynamic cast to be sure.
        if (!try_bind_all<32>(i_obj, t_obj) && !try_bind_all<64>(i_obj, t_obj)) {
            SCP_FATAL(())("No bind found for: {} to {}", i_obj->name(), t_obj->name());
        }
    }

    void order_bind(sc_core::sc_module* module_obj, const std::string& target_order_str,
                    const std::string& initiator_name)
    {
        if (!target_order_str.empty()) {
            std::vector<std::string> target_vec = parse_order_str(target_order_str);
            for (const auto& target : target_vec) {
                std::string target_name = target;
                if (target_name.at(0) != '&') continue;
                target_name.erase(0, 1);
                target_name = std::string(name()) + "." + target_name;
                SCP_TRACE(())("Bind {} to {}", target_name, initiator_name);
                do_binding(module_obj, target_name, initiator_name);
            }
        }
    }

    virtual void do_reset(bool value)
    {
        if (value) SCP_WARN(()) << "Reset";
    }
};

/**
 * ModulesConstruct() function is called at Container constructor.
 */
class Container : public ContainerBase
{
public:
    SCP_LOGGER(());
    Container(const sc_core::sc_module_name& n): ContainerBase(n, false, nullptr)
    {
        SCP_DEBUG(()) << "Container constructor";
        assert(std::string(moduletype.get_value()) == "Container");
    }

    virtual ~Container() = default;
};
/**
 * ModulesConstruct() function is called at Container constructor.
 * p_container_mod_arg has to be passed as a constructor arg.
 */
class ContainerWithArgs : public ContainerBase
{
public:
    SCP_LOGGER(());
    ContainerWithArgs(const sc_core::sc_module_name& n, sc_core::sc_object* p_container_mod_arg)
        : ContainerBase(n, false, p_container_mod_arg)
    {
        SCP_DEBUG(()) << "ContainerWithArgs constructor";
        assert(std::string(moduletype.get_value()) == "ContainerWithArgs");
    }

    virtual ~ContainerWithArgs() = default;
};
/**
 * ModulesConstruct() function should be called explicitly by the user of the class.
 * The function is not called at ContainerDeferModulesConstruct constructor.
 */
class ContainerDeferModulesConstruct : public ContainerBase
{
public:
    SCP_LOGGER(());
    ContainerDeferModulesConstruct(const sc_core::sc_module_name& n): ContainerBase(n, true, nullptr)
    {
        SCP_DEBUG(()) << "ContainerDeferModulesConstruct constructor";
        assert(std::string(moduletype.get_value()) == "ContainerDeferModulesConstruct");
    }

    virtual ~ContainerDeferModulesConstruct() = default;
};

} // namespace ModuleFactory
} // namespace gs

typedef gs::ModuleFactory::Container Container;
typedef gs::ModuleFactory::ContainerDeferModulesConstruct ContainerDeferModulesConstruct;
typedef gs::ModuleFactory::ContainerWithArgs ContainerWithArgs;
GSC_MODULE_REGISTER(Container);
GSC_MODULE_REGISTER(ContainerDeferModulesConstruct);
GSC_MODULE_REGISTER(ContainerWithArgs, sc_core::sc_object*);
#endif
