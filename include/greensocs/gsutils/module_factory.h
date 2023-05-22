
/*
 *  Copyright (C) 2023  Qualcomm
 *
 */

#ifndef GREENSOCS_MODULE_FACTORY_H
#define GREENSOCS_MODULE_FACTORY_H

#include "cciutils.h"
#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <scp/report.h>
#include <scp/helpers.h>

#include <greensocs/libgsutils.h>
#include <greensocs/libgssync.h>

#include <libqbox/qemu-instance.h>
#include <libqbox/ports/target.h>
#include <libqbox/ports/initiator.h>
#include <libqbox/ports/target-signal-socket.h>
#include <libqbox/ports/initiator-signal-socket.h>

#define CCI_GS_MF_NAME "__GS.ModuleFactory."
/**
 * @brief Helper macro to register an sc_module constructor, complete with its (typed) arguments
 *
 */
namespace gs {
namespace ModuleFactory {
static std::vector<std::function<cci::cci_param<gs::cci_constructor_vl>*()>>* GetAvailableModuleList()
{
    static std::vector<std::function<cci::cci_param<gs::cci_constructor_vl>*()>> list;
    return &list;
}
struct ModuleRegistrationWrapper {
    ModuleRegistrationWrapper(std::function<cci::cci_param<gs::cci_constructor_vl>*()> fn)
    {
        auto mods = gs::ModuleFactory::GetAvailableModuleList();
        mods->push_back(fn);
    }
};
} // namespace ModuleFactory
} // namespace gs

#define GSC_MODULE_REGISTER(__NAME__, ...)                                                                             \
    struct GS_MODULEFACTORY_moduleReg_##__NAME__ {                                                                     \
        GS_MODULEFACTORY_moduleReg_##__NAME__() {}                                                                     \
        static gs::ModuleFactory::ModuleRegistrationWrapper& get()                                                     \
        {                                                                                                              \
            static gs::ModuleFactory::ModuleRegistrationWrapper inst([]() -> cci::cci_param<gs::cci_constructor_vl>* { \
                return new cci::cci_param<gs::cci_constructor_vl>(                                                     \
                    CCI_GS_MF_NAME #__NAME__,                                                                          \
                    gs::cci_constructor_vl::FactoryMaker<__NAME__, ##__VA_ARGS__>(#__NAME__), "default constructor",   \
                    cci::CCI_ABSOLUTE_NAME);                                                                           \
            });                                                                                                        \
            return inst;                                                                                               \
        }                                                                                                              \
    };                                                                                                                 \
    static struct gs::ModuleFactory::ModuleRegistrationWrapper&                                                        \
        GS_MODULEFACTORY_moduleReg_##__NAME__inst = GS_MODULEFACTORY_moduleReg_##__NAME__::get()
/**
 * @brief CCI value converted
 * notice that NO conversion is provided.
 *
 */
template <>
struct cci::cci_value_converter<gs::cci_constructor_vl> {
    typedef gs::cci_constructor_vl type;
    static bool pack(cci::cci_value::reference dst, type const& src)
    {
        dst.set_string(src.type);
        return true;
    }
    static bool unpack(type& dst, cci::cci_value::const_reference src)
    {
        if (!src.is_string()) {
            return false;
        }
        std::string moduletype;
        if (!src.try_get(moduletype)) {
            return false;
        }
        cci::cci_param_typed_handle<gs::cci_constructor_vl> m_fac(
            cci::cci_get_broker().get_param_handle(CCI_GS_MF_NAME + moduletype));
        if (!m_fac.is_valid()) {
            SC_REPORT_ERROR("ModuleFactory", ("Can't find module type: " + moduletype).c_str());
            return false;
        }
        dst = *m_fac;

        return true;
    }
};
namespace gs {
namespace ModuleFactory {

SC_MODULE (Container) {
    /**
     * @brief construct a module using the pre-register CCI functor, with typed arguments from a CCI
     * value list. The functor is expected to call new.
     *
     * @param moduletype The name of the type of module to be constructed
     * @param name The name to be given to this instance of the module
     * @param args The CCI list of arguments passed to the constructor (which will be type punned)
     * @return sc_core::sc_module* The module constructed.
     */

    SCP_LOGGER(());

    sc_core::sc_module* construct_module(std::string moduletype, sc_core::sc_module_name name, cci::cci_value_list args)
    {
        gs::cci_constructor_vl m_fac;
        cci::cci_value v = cci::cci_value::from_json("\"" + moduletype + "\"");
        if (!v.try_get<gs::cci_constructor_vl>(m_fac)) {
            SC_REPORT_ERROR("ModuleFactory", ("Can't find module type: " + moduletype).c_str());
        }
        return (m_fac)(name, args);
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

            args.push_back(cci::cci_get_broker().get_preset_cci_value(modulename + ".args." + std::to_string(i)));
        }
        return args;
    }

    template <typename I, typename T>
    bool try_bind(sc_core::sc_object * i_obj, sc_core::sc_object * t_obj)
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
    void name_bind(sc_core::sc_module * m)
    {
        cci::cci_broker_handle m_broker = cci::cci_get_broker();

        for (auto param : sc_cci_children(m->name())) {
            /* We should have a param like
             *  foo = &a.baa
             * foo should relate to an object in this model. a.b.bar should relate to an other
             * object Note that the names are relative to the current module (this).
             */

            std::string targetname = std::string(m->name()) + "." + param;
            if (m_broker.get_preset_cci_value(targetname + ".0").is_string()) {
                // deal with an alias
                std::string src = m_broker.get_preset_cci_value(targetname + ".0").get_string();
                m_broker.set_preset_cci_value(targetname + ".address", m_broker.get_preset_cci_value(src + ".address"));
                m_broker.set_preset_cci_value(targetname + ".size", m_broker.get_preset_cci_value(src + ".size"));
            }
            std::string bindname = targetname + ".bind";
            if (!m_broker.get_preset_cci_value(bindname).is_string()) {
                continue;
            }
            std::string initname = m_broker.get_preset_cci_value(bindname).get_string();

            if (initname.at(0) != '&') continue; // all port binds are donated with a '&', so not for us

            initname.erase(0, 1); // remove leading '&'
            initname = std::string(name()) + "." + initname;

            SCP_TRACE(())("Bind {} to {}", targetname, initname);

            sc_core::sc_object* i_obj = nullptr;
            sc_core::sc_object* t_obj = nullptr;
            /* find the initiator object */

            i_obj = find_sc_obj(nullptr, initname);

            /* find the target object which mast be inside this object */
            t_obj = find_sc_obj(m, targetname);

            // actually you could probably just cast everything to (tlm::tlm_target_socket<>*), but
            // we will dynamic cast to be sure.

            while (1) {
                if (try_bind<tlm_utils::multi_init_base<>, tlm::tlm_base_target_socket<>>(i_obj, t_obj)) break;
                if (try_bind<tlm_utils::multi_init_base<>, tlm_utils::multi_target_base<>>(i_obj, t_obj)) break;
                if (try_bind<tlm_utils::multi_init_base<>, QemuTargetSocket<>>(i_obj, t_obj)) break;
                if (try_bind<tlm::tlm_base_initiator_socket<>, tlm::tlm_base_target_socket<>>(i_obj, t_obj)) break;
                if (try_bind<tlm::tlm_base_initiator_socket<>, tlm_utils::multi_target_base<>>(i_obj, t_obj)) break;
                if (try_bind<tlm::tlm_base_initiator_socket<>, QemuTargetSocket<>>(i_obj, t_obj)) break;
                if (try_bind<tlm::tlm_base_initiator_socket<32, tlm::tlm_fw_transport_if<>, tlm::tlm_bw_transport_if<>,
                                                            1, sc_core::SC_ZERO_OR_MORE_BOUND>,
                             tlm_utils::multi_target_base<>>(i_obj, t_obj))
                    break;
                if (try_bind<QemuInitiatorSocket<>, tlm::tlm_base_target_socket<>>(i_obj, t_obj)) break;
                if (try_bind<QemuInitiatorSocket<>, tlm_utils::multi_target_base<>>(i_obj, t_obj)) break;
                if (try_bind<QemuInitiatorSocket<>, QemuTargetSocket<>>(i_obj, t_obj)) break;
                if (try_bind<QemuInitiatorSignalSocket, QemuTargetSignalSocket>(i_obj, t_obj)) break;
                if (try_bind<QemuInitiatorSignalSocket, TargetSignalSocket<bool>>(i_obj, t_obj)) break;
                if (try_bind<InitiatorSignalSocket<bool>, QemuTargetSignalSocket>(i_obj, t_obj)) break;
                if (try_bind<InitiatorSignalSocket<bool>, TargetSignalSocket<bool>>(i_obj, t_obj)) break;
                SCP_FATAL(())("No bind found for: {} to {}", i_obj->name(), t_obj->name());
            }
        }
    }

    std::list<std::string> PriorityConstruct(void)
    {
        cci::cci_broker_handle m_broker = cci::cci_get_broker();

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

    void ModulesConstruct(void)
    {
        cci::cci_broker_handle m_broker = cci::cci_get_broker();

        std::list<sc_core::sc_module*> allModules;

        for (auto name : PriorityConstruct()) {
            if (m_broker.has_preset_value(std::string(sc_module::name()) + "." + name + ".moduletype")) {
                if (m_broker.has_preset_value(std::string(sc_module::name()) + "." + name + ".dont_construct")) {
                    sc_core::sc_object* no_construct_mod_obj = nullptr;
                    std::string no_construct_mod_name = std::string(sc_module::name()) + "." + name;
                    no_construct_mod_obj = gs::find_sc_obj(nullptr, no_construct_mod_name);
                    auto no_construct_mod = dynamic_cast<sc_core::sc_module*>(no_construct_mod_obj);
                    if (!no_construct_mod) {
                        SCP_FATAL(()) << "The object " << std::string(sc_module::name()) + "." + name
                                      << " is not yet constructed";
                    }
                    allModules.push_back(no_construct_mod);
                } else {
                    std::string type = m_broker
                                           .get_preset_cci_value(std::string(sc_module::name()) + "." + name +
                                                                 ".moduletype")
                                           .get_string();
                    std::cout << "adding a " << type << " with name " << std::string(sc_module::name()) + "." + name
                              << "\n";

                    cci::cci_value_list mod_args = get_module_args(std::string(sc_module::name()) + "." + name);
                    std::cout << mod_args.size() << " arguments found for " << type << "\n";
                    sc_core::sc_module* m = construct_module(type, name.c_str(), mod_args);

                    if (!m) {
                        SC_REPORT_ERROR("ModuleFactory",
                                        ("Can't automatically handle argument list for module: " + type).c_str());
                    }

                    allModules.push_back(m);
                }
            } // else it's some other config
        }
        // bind everything
        for (auto mod : allModules) {
            name_bind(mod);
        }
    }

    cci_param<std::string> moduletype; // for consistency
    /**
     * @brief construct a Container, and all modules within it, and perform binding
     *
     * @param _n name to give the container
     */
    std::vector<cci::cci_param<gs::cci_constructor_vl>*> registered_mods;

    Container(const sc_core::sc_module_name _n)
        : moduletype("moduletype", "", "Module type for the TLM container, must be \"Container\"")
    {
        assert((std::string)moduletype == "Container");
        auto mods = gs::ModuleFactory::GetAvailableModuleList();

        while (mods->size()) {
            registered_mods.push_back((mods->back())());
            mods->pop_back();
        }

        ModulesConstruct();
    }
};
GSC_MODULE_REGISTER(Container);

} // namespace ModuleFactory
} // namespace gs

#endif
