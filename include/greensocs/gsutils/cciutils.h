/*
 *  Copyright (C) 2020  GreenSocs
 *
 */

#ifndef CCIUTILS_H
#define CCIUTILS_H

#include <iostream>
#include <list>

#include "luafile_tool.h"
#include <cci_configuration>
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc>
#include <tlm>

/*********************
 * CCI Convenience
 *********************/

namespace gs {
using namespace cci;

/* return the leaf name of the given module name */
static std::string sc_cci_leaf_name(std::string name)
{
    return name.substr(name.find_last_of(".") + 1);
}

/* return a list of children from the given module name, can be used inside
 * or outside the heirarchy */
static std::list<std::string> sc_cci_children(sc_core::sc_module_name name)
{
    cci_broker_handle m_broker = (sc_core::sc_get_current_object())
        ? cci_get_broker()
        : cci_get_global_broker(cci_originator("gs__sc_cci_children"));
    std::list<std::string> children;
    int l = strlen(name) + 1;
    auto uncon = m_broker.get_unconsumed_preset_values(
        [&name](const std::pair<std::string, cci_value>& iv) {
            return iv.first.find(std::string(name) + ".") == 0;
        });
    for (auto p : uncon) {
        children.push_back(p.first.substr(l, p.first.find(".", l) - l));
    }
    children.sort();
    children.unique();
    return children;
}

class ConfigurableBroker : public cci_utils::consuming_broker {

public:
    // a set of perameters that should be exposed up the broker stack
    std::set<std::string> expose;
    cci_param<std::string> conf_file;

private:
    bool has_parent;
    cci_broker_if& m_parent;
    std::string m_orig_name;
    cci_originator m_originator;

    // convenience function for constructor
    cci_broker_if& get_parent_broker()
    {
        if (sc_core::sc_get_current_object()) {
            has_parent = true;
            return unwrap_broker(cci_get_broker());
        } else {
            // We ARE the global broker
            has_parent = false;
            return *this;
        }
    }
    std::string hierarchical_name()
    {
        if (!sc_core::sc_get_current_object()) {
            return std::string("");
        } else {
            return cci_originator().name();
        }
    }
    cci_originator get_cci_originator(const char* n)
    {
        if (!sc_core::sc_get_current_object()) {
            return cci_originator(n);
        } else {
            return cci_originator();
        }
    }
    /*
   * private function to determine if we send to the parent broker or not
   */
    bool sendToParent(const std::string& parname) const
    {
        return ((expose.find(parname) != expose.end()) && (!is_global_broker()));
    }

    /*
   * Expose all params that have not been configured
   */

    void
    initialize_params(const std::initializer_list<cci_name_value_pair>& list)
    {
        using namespace cci;

        // Expose everything
        for (auto p : m_parent.get_unconsumed_preset_values()) {
            expose.insert(p.first);
        }
        // Hide configured things
        for (auto& p : list) {
            expose.erase(relname(p.first));
            set_preset_cci_value(relname(p.first), p.second, m_originator);
        }
    }

public:
    /*
 * public interface functions
 */

    void print_help()
    {
        std::cout << std::endl
                  << "Available Configuration Parameters:" << std::endl
                  << "===================================" << std::endl;
        for (auto p : get_param_handles(get_cci_originator("Command line help"))) {
            std::cout << p.name() << " : " << p.get_description() << " (configured value " << p.get_cci_value() << ") " << std::endl;
        }
        std::cout << std::endl;
    }
/*
 * default constructor:
 * When constructed with no initialised parameters, it is assumed that ALL
 * parameters are to be treated as private
 */
#define BROKERNAME "gs::ConfigurableBroker"
    ConfigurableBroker(const std::string& name = BROKERNAME,
        bool load_conf_file = true)
        : m_orig_name(hierarchical_name())
        , m_originator(get_cci_originator(name.c_str()))
        , consuming_broker(hierarchical_name() + name)
        , conf_file("lua_conf", "",
              cci_broker_handle(get_parent_broker(),
                  get_cci_originator(name.c_str())),
              "Local lua configuration file", CCI_RELATIVE_NAME,
              get_cci_originator(name.c_str()))
        , m_parent(get_parent_broker()) // local convenience function
    {
        cci_register_broker(this);

        if (load_conf_file && !(std::string(conf_file).empty())) {
            LuaFile_Tool lua("lua", m_orig_name);
            lua.config(std::string(conf_file).c_str());
        }
    }

    /*
   * Constructor with just boolean, for convenience
   */
    ConfigurableBroker(bool load_conf_file)
        : ConfigurableBroker(BROKERNAME, load_conf_file)
    {
    }

    /*
   * initialised list constructor:
   * When constructed with a list of initialised parameters, all other params
   * will be exported to the parent broker
   */
    ConfigurableBroker(std::initializer_list<cci_name_value_pair> list,
        bool load_conf_file = true)
        : ConfigurableBroker(false)
    {
        initialize_params(list);

        if (load_conf_file && !(std::string(conf_file).empty())) {
            LuaFile_Tool lua("lua", m_orig_name);
            lua.config(std::string(conf_file).c_str());
        }
    }

    /*
   * in this case, the expectation is that this is being used at (or near) the
   * top level of the design, and this broker will act as a global broker. A
   * list of values will be used to set default values, but will be overwritten
   * by configuration passed on the command line
   */
    ConfigurableBroker(const int argc, char* const argv[],
        std::initializer_list<cci_name_value_pair> list = {},
        bool load_conf_file = true)
        : ConfigurableBroker(false)
    {

        for (auto& p : list) {
            set_preset_cci_value(relname(p.first), p.second, m_originator);
        }

        LuaFile_Tool lua("lua", m_orig_name);
        lua.parseCommandLine(argc, argv);

        static const char* optstring = "h";
        static struct option long_options[] = {
            { "help", 0, 0, 'h' }, // '--help' = '-h'
            { 0, 0, 0, 0 }
        };
        optind = 1;
        while (1) {
            int c = getopt_long(argc, argv, optstring, long_options, 0);
            if (c == EOF)
                break;
            switch (c) {
            case 'h': // -h and --help
                sc_core::sc_spawn_options opts;
                sc_core::sc_spawn([&]() { print_help();sc_core::sc_stop(); }, "print_help", &opts);
                break;
            }
        }
        optind = 1;

        /* check to see if the conf_file was set ! */
        if (load_conf_file && !(std::string(conf_file).empty())) {
            LuaFile_Tool lua("lua", m_orig_name);
            lua.config(std::string(conf_file).c_str());
        }
    }

    std::string relname(const std::string& n) const
    {
        return m_orig_name + std::string(".") + n;
    }

    ~ConfigurableBroker() {}

    cci_originator get_value_origin(const std::string& parname) const
    {
        if (sendToParent(parname)) {
            return m_parent.get_value_origin(parname);
        } else {
            return consuming_broker::get_value_origin(parname);
        }
    }

    /* NB  missing from upstream CCI 'broker' see
   * https://github.com/OSCI-WG/cci/issues/258 */
    bool has_preset_value(const std::string& parname) const
    {
        if (sendToParent(parname)) {
            return m_parent.has_preset_value(parname);
        } else {
            return consuming_broker::has_preset_value(parname);
        }
    }

    cci_value get_preset_cci_value(const std::string& parname) const
    {
        if (sendToParent(parname)) {
            return m_parent.get_preset_cci_value(parname);
        } else {
            return consuming_broker::get_preset_cci_value(parname);
        }
    }

    void lock_preset_value(const std::string& parname)
    {
        if (sendToParent(parname)) {
            return m_parent.lock_preset_value(parname);
        } else {
            return consuming_broker::lock_preset_value(parname);
        }
    }

    cci_value get_cci_value(const std::string& parname) const
    {
        if (sendToParent(parname)) {
            return m_parent.get_cci_value(parname);
        } else {
            return consuming_broker::get_cci_value(parname);
        }
    }

    void add_param(cci_param_if* par)
    {
        if (sendToParent(par->name())) {
            return m_parent.add_param(par);
        } else {
            return consuming_broker::add_param(par);
        }
    }

    void remove_param(cci_param_if* par)
    {
        if (sendToParent(par->name())) {
            return m_parent.remove_param(par);
        } else {
            return consuming_broker::remove_param(par);
        }
    }

    // Functions below here require an orriginator to be passed to the local
    // method variant.

    void set_preset_cci_value(const std::string& parname,
        const cci_value& cci_value,
        const cci_originator& originator)
    {
        if (sendToParent(parname)) {
            return m_parent.set_preset_cci_value(parname, cci_value, originator);
        } else {
            return consuming_broker::set_preset_cci_value(parname, cci_value,
                originator);
        }
    }
    cci_param_untyped_handle
    get_param_handle(const std::string& parname,
        const cci_originator& originator) const
    {
        if (sendToParent(parname)) {
            return m_parent.get_param_handle(parname, originator);
        }
        cci_param_if* orig_param = get_orig_param(parname);
        if (orig_param) {
            return cci_param_untyped_handle(*orig_param, originator);
        }
        if (has_parent) {
            return m_parent.get_param_handle(parname, originator);
        }
        return cci_param_untyped_handle(originator);
    }

    std::vector<cci_param_untyped_handle>
    get_param_handles(const cci_originator& originator) const
    {
        if (has_parent) {
            std::vector<cci_param_untyped_handle> p_param_handles = m_parent.get_param_handles();
            std::vector<cci_param_untyped_handle> param_handles = consuming_broker::get_param_handles(originator);
            // this is likely to be more efficient the other way round, but it keeps
            // things consistent and means the local (more useful) params will be at
            // the head of the list.
            param_handles.insert(param_handles.end(), p_param_handles.begin(),
                p_param_handles.end());
            return param_handles;
        } else {
            return consuming_broker::get_param_handles(originator);
        }
    }

    bool is_global_broker() const { return (!has_parent); }
};
} // namespace gs

#endif // CCIUTILS_H
