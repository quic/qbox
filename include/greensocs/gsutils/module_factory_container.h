
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
#include <tlm_utils/simple_initiator_socket.h>
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

#include <greensocs/systemc-uarts/backends/char-bf/stdio.h>
#include <greensocs/gsutils/ports/biflow-socket.h>
#include <greensocs/gsutils/module_factory_registery.h>
#include <greensocs/base-components/transaction_forwarder_if.h>

#define ModuleFactory_CONTAINER_BUSWIDTH 32
namespace gs {
namespace ModuleFactory {

class ContainerBase : public sc_core::sc_module, public transaction_forwarder_if<CONTAINER>
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

            auto arg = cci::cci_get_broker().get_preset_cci_value(modulename + ".args." + std::to_string(i));
            if (arg.is_string() && std::string(arg.get_string())[0] == '&') {
                std::string parent = std::string(modulename).substr(0, modulename.find_last_of("."));
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
                if (try_bind<gs::biflow_bindable, gs::biflow_bindable>(i_obj, t_obj)) break;
                if (try_bind<tlm::tlm_base_initiator_socket<32, tlm::tlm_fw_transport_if<>, tlm::tlm_bw_transport_if<>,
                                                            1, sc_core::SC_ZERO_OR_MORE_BOUND>,
                             tlm::tlm_base_target_socket<>>(i_obj, t_obj))
                    break;
                if (try_bind<
                        tlm::tlm_initiator_socket<32, tlm::tlm_base_protocol_types, 1, sc_core::SC_ONE_OR_MORE_BOUND>,
                        tlm::tlm_target_socket<32, tlm::tlm_base_protocol_types, 1, sc_core::SC_ZERO_OR_MORE_BOUND>>(
                        i_obj, t_obj))
                    break;
                if (try_bind<tlm_utils::multi_init_base<>, tlm::tlm_target_socket<32, tlm::tlm_base_protocol_types, 1,
                                                                                  sc_core::SC_ZERO_OR_MORE_BOUND>>(
                        i_obj, t_obj))
                    break;
                 if (try_bind<tlm::tlm_base_initiator_socket<32, tlm::tlm_fw_transport_if<>, tlm::tlm_bw_transport_if<>,
                                                            1, sc_core::SC_ZERO_OR_MORE_BOUND>,
                             tlm::tlm_target_socket<32, tlm::tlm_base_protocol_types, 1, sc_core::SC_ZERO_OR_MORE_BOUND>>(i_obj, t_obj))
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

                    SCP_INFO(()) << "Adding a " << type << " with name " << std::string(sc_module::name()) + "." + name;

                    cci::cci_value_list mod_args = get_module_args(std::string(sc_module::name()) + "." + name);
                    SCP_INFO(()) << mod_args.size() << " arguments found for " << type;
                    sc_core::sc_module* m = construct_module(type, name.c_str(), mod_args);

                    if (!m) {
                        SC_REPORT_ERROR("ModuleFactory",
                                        ("Can't automatically handle argument list for module: " + type).c_str());
                    }

                    allModules.push_back(m);
                    if (!m_local_pass) {
                        m_local_pass = dynamic_cast<transaction_forwarder_if<PASS>*>(m);
                    }
                }
            } // else it's some other config
        }
        // bind everything
        for (auto mod : allModules) {
            name_bind(mod);
        }
    }

    using tlm_initiator_socket_type = tlm_utils::simple_initiator_socket_b<
        ContainerBase, ModuleFactory_CONTAINER_BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_type = tlm_utils::simple_target_socket_tagged_b<
        ContainerBase, ModuleFactory_CONTAINER_BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;

private:
    bool m_defer_modules_construct;

public:
    cci::cci_broker_handle m_broker;
    cci_param<std::string> moduletype; // for consistency
    cci::cci_param<uint32_t> p_tlm_initiator_ports_num;
    cci::cci_param<uint32_t> p_tlm_target_ports_num;
    cci::cci_param<uint32_t> p_initiator_signals_num;
    cci::cci_param<uint32_t> p_target_signals_num;
    sc_core::sc_vector<tlm_initiator_socket_type> initiator_sockets;
    sc_core::sc_vector<tlm_target_socket_type> target_sockets;
    sc_core::sc_vector<InitiatorSignalSocket<bool>> initiator_signal_sockets;
    sc_core::sc_vector<TargetSignalSocket<bool>> target_signal_sockets;
    transaction_forwarder_if<PASS>* m_local_pass;

    /**
     * @brief construct a Container, and all modules within it, and perform binding
     *
     * @param _n name to give the container
     */
    std::vector<cci::cci_param<gs::cci_constructor_vl>*> registered_mods;

    ContainerBase(const sc_core::sc_module_name _n, bool defer_modules_construct)
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
        , m_local_pass(nullptr)
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
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        if (m_local_pass) {
            return m_local_pass->fw_get_direct_mem_ptr(id, trans, dmi_data);
        }
    }

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        if (m_local_pass) {
            m_local_pass->fw_invalidate_direct_mem_ptr(start, end);
        }
    }
};

/**
 * ModulesConstruct() function is called at Container constructor.
 */
class Container : public ContainerBase
{
public:
    SCP_LOGGER(());
    Container(const sc_core::sc_module_name& n): ContainerBase(n, false)
    {
        SCP_DEBUG(()) << "Container constructor";
        assert(std::string(moduletype.get_value()) == "Container");
    }

    virtual ~Container() = default;
};
/**
 * ModulesConstruct() function should be called explicitly by the user of the class.
 * The function is not called at ContainerDeferModulesConstruct constructor.
 */
class ContainerDeferModulesConstruct : public ContainerBase
{
public:
    SCP_LOGGER(());
    ContainerDeferModulesConstruct(const sc_core::sc_module_name& n): ContainerBase(n, true)
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
GSC_MODULE_REGISTER(Container);
GSC_MODULE_REGISTER(ContainerDeferModulesConstruct);
#endif
