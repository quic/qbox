/*
 * Copyright (c) 2022 GreenSocs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CCIUTILS_H
#define CCIUTILS_H

#include <iostream>
#include <list>
#include <regex>

#include "luafile_tool.h"
#include "registers.h"

#include <cci_configuration>
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc>
#include <tlm>

/*********************
 * CCI Convenience
 *********************/

namespace gs {
using namespace cci;

/**
 * @brief Helper function to find a sc_object by fully qualified name
 *  throws SC_ERROR if nothing found.
 * @param m     current parent object (use null to start from the top)
 * @param name  name being searched for
 * @return sc_core::sc_object* return object if found
 */
static sc_core::sc_object* find_sc_obj(sc_core::sc_object* m, std::string name, bool test = false) {
    if (m && name == m->name()) {
        return m;
    }

    std::vector<sc_core::sc_object*> children;
    if (m) {
        children = m->get_child_objects();
    } else {
        children = sc_core::sc_get_top_level_objects();
    }

    auto ip_it = std::find_if(begin(children), end(children), [&name](sc_core::sc_object* o) {
        return (name.compare(0, strlen(o->name()), o->name()) == 0) && find_sc_obj(o, name, true);
    });
    if (ip_it != end(children)) {
        return find_sc_obj(*ip_it, name);
    } else {
        if (!test) {
            if (m) {
                SCP_ERR("cciutils.find_sc_obj")
                    << "Unable to find " << name << " in sc_object " << m->name();
            } else {
                SCP_ERR("cciutils.find_sc_obj") << "Unable to find " << name;
            }
        }
    }
    return nullptr;
}

/**
 * @brief return the leaf name of the given systemc hierarchical name
 *
 * @param name
 * @return std::string
 */
static std::string sc_cci_leaf_name(std::string name) {
    return name.substr(name.find_last_of(".") + 1);
}

/**
 * @brief return a list of 'unconsumed' children from the given module name, can be used inside
 * or outside the heirarchy
 *
 * @param name
 * @return std::list<std::string>
 */
static std::list<std::string> sc_cci_children(sc_core::sc_module_name name) {
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

/**
 * @brief return a list of 'unconsumed' items in the module matching the base name given, plus '_'
 * and a numeric value typically this would be a list of items from an sc_vector Can be used inside
 * or outside the heirarchy
 *
 * @param module_name : name of the module
 * @param list_name : base name of the list (e.g. name of the sc_vector)
 * @return std::list<std::string>
 */
static std::list<std::string> sc_cci_list_items(sc_core::sc_module_name module_name,
                                                std::string list_name) {
    cci_broker_handle m_broker = (sc_core::sc_get_current_object())
                                     ? cci_get_broker()
                                     : cci_get_global_broker(cci_originator("gs__sc_cci_children"));
    std::string name = std::string(module_name) + "." + list_name;
    std::regex search(name + "_[0-9]+(\\.|$)");
    std::list<std::string> children;
    int l = strlen(module_name) + 1;
    auto uncon = m_broker.get_unconsumed_preset_values(
        [&search](const std::pair<std::string, cci::cci_value>& iv) {
            return std::regex_search(iv.first, search);
        });
    for (auto p : uncon) {
        std::smatch match;
        std::regex_search(p.first, match, search);
        children.push_back(p.first.substr(l, (match.length() - l) - (match.str().back() == '.' ? 1 : 0)));
    }
    children.sort();
    children.unique();
    return children;
}

template <typename T>
T cci_get(std::string name) {
    auto m_broker = cci::cci_get_broker();
    m_broker.ignore_unconsumed_preset_values(
        [name](const std::pair<std::string, cci::cci_value>& iv) -> bool {
            return iv.first == name;
        });
    m_broker.lock_preset_value(name);
    T ret;
    if (!m_broker.get_preset_cci_value(name).template try_get<T>(ret)) {
        SCP_ERR("cciutils.cci_get") << "Unable to get parameter " << name << "\nIs your .lua file up-to-date?";
    };
    return ret;
}

/**
 * @brief Configurable Broker class, inherits from the 'standard' cci_utils broker, but adds
 * 1/ The ability to be instanced in the constructor
 * 2/ The ability to automatically load a configuration file
 * 3/ Explicitly set parameters (from the constructor) are 'hidden' from the parent broker
 */
class ConfigurableBroker : public virtual cci_utils::consuming_broker
{
private:
    std::set<std::string> m_unignored; // before conf_file

public:
    // a set of perameters that should be exposed up the broker stack
    std::set<std::string> hide;
    cci_param<std::string> conf_file;

private:
    bool has_parent;
    cci_broker_if& m_parent;
    std::string m_orig_name;
    cci_originator m_originator;
    cci_param<ConfigurableBroker*>* m_child_ref;

    friend class cci_value_converter<ConfigurableBroker>;

    // convenience function for constructor
    cci_broker_if& get_parent_broker() {
        if (sc_core::sc_get_current_object()) {
            has_parent = true;
            return unwrap_broker(cci_get_broker());
        } else {
            // We ARE the global broker
            has_parent = false;
            return *this;
        }
    }
    std::string hierarchical_name() {
        if (!sc_core::sc_get_current_object()) {
            return std::string("");
        } else {
            return cci_originator().name();
        }
    }
    cci_originator get_cci_originator(const char* n) {
        if (!sc_core::sc_get_current_object()) {
            return cci_originator(n);
        } else {
            return cci_originator();
        }
    }
    /**
     * @brief private function to determine if we send to the parent broker or not
     *
     */
    bool sendToParent(const std::string& parname) const {
        return ((hide.find(parname) == hide.end()) && (!is_global_broker()));
    }

    /**
     * @brief Expose all params that have not been configured
     *
     * @param list
     */
    void initialize_params(const std::initializer_list<cci_name_value_pair>& list) {
        using namespace cci;

        for (auto& p : list) {
            hide.insert(relname(p.first));
            if (!p.second.is_null()) {
                set_preset_cci_value(relname(p.first), p.second, m_originator);
            }
        }
    }

    void alias_params(const std::initializer_list<std::pair<std::string, std::string>>& list) {
        // handle aliases
        for (auto& p : list) {
            std::string aliasname = relname(p.second);
            set_preset_cci_value(relname(p.first), get_preset_cci_value(aliasname), m_originator);
            alias_param(relname(p.first), aliasname);
        }
    }
    /**
     *  @fn     void typed_post_write_callback(const cci::cci_param_write_event<int> & ev)
     *  @brief  Post Callback function to sync ongoing written parameter value with synced_handle
     * value
     *  @return void
     */
    void untyped_post_write_callback(const cci::cci_param_write_event<>& ev,
                                     cci::cci_param_handle synced_handle) {
        synced_handle.set_cci_value(ev.new_value);
    }
    /**
     *  @fn     void sync_values(cci::cci_param_handle _param_handle_1, cci::cci_param_handle
     * _param_handle_2)
     *  @brief  Function for synchronizing the values of cci_parameter of OWNER
     *          modules via the PARAM_VALUE_SYNC
     *  @param  _param_handle_1 The first parameter to be synced
     *  @param  _param_handle_2 The second parameter to be synced
     *  @return void
     */
    void sync_values(cci::cci_param_handle _param_handle_1, cci::cci_param_handle _param_handle_2) {
        // In order to synchronize even the default values of the owner modules,
        // use cci_base_param of one parameter as reference, write the same value
        // to the other pararmeter's cci_base_param using JSON
        _param_handle_1.set_cci_value(_param_handle_2.get_cci_value());

        post_write_cb_vec.push_back(_param_handle_1.register_post_write_callback(
            sc_bind(&ConfigurableBroker::untyped_post_write_callback, this, sc_unnamed::_1,
                    _param_handle_2)));

        post_write_cb_vec.push_back(_param_handle_2.register_post_write_callback(
            sc_bind(&ConfigurableBroker::untyped_post_write_callback, this, sc_unnamed::_1,
                    _param_handle_1)));
    }

    std::vector<cci::cci_callback_untyped_handle> post_write_cb_vec; ///< Callback Adaptor Objects

    cci_param_create_callback_handle register_cb;
    std::vector<std::pair<std::string, std::string>> alias_list;

    void alias_param(std::string a, std::string b) {
        alias_list.push_back(std::make_pair(a, b));
        if (register_cb == cci_param_create_callback_handle()) {
            register_cb = register_create_callback(
                sc_bind(&ConfigurableBroker::alias_param_callback, this, sc_unnamed::_1),
                m_originator);
        }
        alias_param_callback(cci::cci_param_handle());
    }

    void alias_param_callback(const cci_param_untyped_handle& ph) {
        alias_list.erase(
            std::remove_if(
                alias_list.begin(), alias_list.end(),
                [&](auto a) {
                    cci_param_untyped_handle h_a = get_param_handle(a.first, m_originator);
                    if (h_a.is_valid() && h_a.get_mutable_type() == cci::CCI_IMMUTABLE_PARAM) {
                        return true;
                    }
                    cci_param_untyped_handle h_b = get_param_handle(a.second, m_originator);
                    if (h_b.is_valid() && !h_a.is_valid()) {
                        set_preset_cci_value(a.first, h_b.get_cci_value(), m_originator);
                        return false; // maybe it'll be mutable when it arrives?
                    }
                    if (h_a.is_valid() && h_b.is_valid()) {
                        sync_values(h_a, h_b);
                        return true;
                    }
                    return false;
                }),
            alias_list.end());

        if (alias_list.size() == 0) {
            unregister_create_callback(register_cb, m_originator);
        }
    }

    class help_helper : public sc_core::sc_module
    {
    public:
        help_helper(sc_core::sc_module_name name) {}
        std::function<void()> help_cb;
        void register_cb(std::function<void()> cb) { help_cb = cb; }
        void end_of_elaboration() {
            if (help_cb)
                help_cb();
        }

        void start_of_simulation() {
            auto m_broker = cci::cci_get_broker();
            // remove lua builtins
            m_broker.ignore_unconsumed_preset_values(
                [](const std::pair<std::string, cci::cci_value>& iv) -> bool {
                    return ((iv.first)[0] == '_' || iv.first == "math.maxinteger") ||
                           (iv.first == "math.mininteger") || (iv.first == "utf8.charpattern");
                });

            auto uncon = m_broker.get_unconsumed_preset_values();
            for (auto p : uncon) {
                SCP_WARN("cciutils.help")
                    << "Params: Unconsumed parameter : " << p.first << " = " << p.second.to_json();
            }

            if (help_cb)
                exit(0);
        }
    };
    help_helper m_help_helper;

public:
    /*
     * public interface functions
     */

    std::vector<cci_name_value_pair> get_consumed_preset_values() const {
        std::vector<cci_name_value_pair> consumed_preset_cci_values;
        std::map<std::string, cci_value>::const_iterator iter;
        std::vector<cci_preset_value_predicate>::const_iterator pred;

        for (iter = m_unused_value_registry.begin(); iter != m_unused_value_registry.end();
             ++iter) {
            for (pred = m_ignored_unconsumed_predicates.begin();
                 pred != m_ignored_unconsumed_predicates.end(); ++pred) {
                const cci_preset_value_predicate& p = *pred; // get the actual predicate
                if (p(std::make_pair(iter->first, iter->second))) {
                    break;
                }
            }
            if (pred != m_ignored_unconsumed_predicates.end()) {
                consumed_preset_cci_values.push_back(std::make_pair(iter->first, iter->second));
            }
        }
        return consumed_preset_cci_values;
    }

    void print_debug(bool top = true) {
        /* NB there is a race condition between this and the QEMU uart which
         * has a tendency to wipe the buffer. We will use cerr here */

        if (top) {
            std::cerr << std::endl
                      << "Available Configuration Parameters:" << std::endl
                      << "===================================" << std::endl;
        }

        for (auto p : get_consumed_preset_values()) {
            std::cerr << "Consumed parameter : " << p.first << " (with value 0x" << std::hex
                      << p.second << ")" << std::endl;
        }

        std::string ending = "childbroker";
        for (auto p : get_param_handles(get_cci_originator("Command line help"))) {
            if (find_register(p.name()))
                continue;

            if (!std::equal(ending.rbegin(), ending.rend(), std::string(p.name()).rbegin())) {
                std::cerr << p.name() << " : " << p.get_description() << " (configured value "
                          << p.get_cci_value() << ") " << std::endl;
            } else {
                cci_param_typed_handle<ConfigurableBroker*> c(p);
                ConfigurableBroker* cc = c.get_value();
                if (cc != this) {
                    cc->print_debug(false);
                }
            }
        }
        if (top) {
            std::cerr << std::endl << "Logging parameters :" << std::endl;
            for (auto p : scp::get_logging_parameters()) {
                std::cerr << p << std::endl;
            }

            std::cerr << "---" << std::endl;
        }
    }

    /**
     * @brief Construct a new Configurable Broker object
     * default constructor:
     * When constructed with no initialised parameters, it is assumed that ALL
     * parameters are to be treated as private
     * @param name Broker name (Default provided)
     * @param load_conf_file : request that configuration file is loaded (default true)
     */
#define BROKERNAME "gs::ConfigurableBroker"
    ConfigurableBroker(const std::string& name = BROKERNAME, bool load_conf_file = true)
        : m_orig_name(hierarchical_name())
        , m_originator(get_cci_originator(name.c_str()))
        , consuming_broker(hierarchical_name() + "." + name)
        , conf_file("lua_conf", "",
                    cci_broker_handle(get_parent_broker(), get_cci_originator(name.c_str())),
                    "Local lua configuration file", CCI_RELATIVE_NAME,
                    get_cci_originator(name.c_str()))
        , m_parent(get_parent_broker()) // local convenience function
        , m_child_ref(NULL)
        , m_help_helper("help_helper") {
        if (has_parent) {
            m_child_ref = new cci_param<ConfigurableBroker*>(
                (hierarchical_name() + "." + name + ".childbroker").c_str(), (this), "");
        }

        cci_register_broker(this);

        if (load_conf_file && !(std::string(conf_file).empty())) {
            LuaFile_Tool lua("lua", std::string(conf_file).c_str(), m_orig_name);
        }
    }

    /**
     * @brief Construct a new Configurable Broker object
     *
     * @param load_conf_file request that configuration file is loaded
     */
    ConfigurableBroker(bool load_conf_file): ConfigurableBroker(BROKERNAME, load_conf_file) {}
    /**
     * @brief Construct a new Configurable Broker object from list.
     *  When constructed with a list of initialised parameters, all other params
     *  will be exported to the parent broker
     * @param list
     * @param load_conf_file
     */
    ConfigurableBroker(std::initializer_list<cci_name_value_pair> list,
                       std::initializer_list<std::pair<std::string, std::string>> alias_list = {},
                       bool load_conf_file = true)
        : ConfigurableBroker(false) {
        initialize_params(list);
        alias_params(alias_list);
        if (load_conf_file && !(std::string(conf_file).empty())) {
            LuaFile_Tool lua("lua", m_orig_name);
            lua.config(std::string(conf_file).c_str());
        }
    }
    /**
     * @brief Construct a new Configurable Broker object, all arguments have default options.
     * in this case, the expectation is that this is being used at (or near) the
     * top level of the design, and this broker will act as a global broker. A
     * list of values will be used to set default values, but will be overwritten
     * by configuration passed on the command line
     * @param argc argc and argv provided, such that they can be scanned for configuration commands
     * @param argv
     * @param list list of pre-configred values (to be hidden)
     * @param load_conf_file
     */
    ConfigurableBroker(const int argc, char* const argv[],
                       std::initializer_list<cci_name_value_pair> list = {},
                       bool load_conf_file = true, bool enforce_config_file = false)
        : ConfigurableBroker(false) {
        for (auto& p : list) {
            set_preset_cci_value(relname(p.first), p.second, m_originator);
        }

        LuaFile_Tool lua("lua", argc, argv, "", enforce_config_file);

        static const char* optstring = "d";
        static struct option long_options[] = { { "debug", 0, 0, 'd' }, // '--debug' = '-d'
                                                { 0, 0, 0, 0 } };

        opterr = 0;
        optind = 1;
        while (1) {
            int c = getopt_long(argc, argv, optstring, long_options, 0);
            if (c == EOF)
                break;
            switch (c) {
            case 'd': // -d and --debug
                sc_core::sc_spawn_options opts;
                m_help_helper.register_cb([&]() -> void { print_debug(); });
                break;
            }
        }
        optind = 1;

        /* check to see if the conf_file was set ! */
        if (load_conf_file && !(std::string(conf_file).empty())) {
            LuaFile_Tool lua("lua", std::string(conf_file).c_str(), m_orig_name);
        }
    }

    ConfigurableBroker(const int argc, char* const argv[], bool enforce_config_file)
        : ConfigurableBroker(argc, argv, {}, true, enforce_config_file) {}

    std::string relname(const std::string& n) const {
        return (m_orig_name.empty()) ? n : m_orig_name + std::string(".") + n;
    }

    ~ConfigurableBroker() {
        if (m_child_ref) {
            delete m_child_ref;
        }
    }

    cci_originator get_value_origin(const std::string& parname) const override {
        if (sendToParent(parname)) {
            return m_parent.get_value_origin(parname);
        } else {
            return consuming_broker::get_value_origin(parname);
        }
    }

    /* NB  missing from upstream CCI 'broker' see
     * https://github.com/OSCI-WG/cci/issues/258 */
    bool has_preset_value(const std::string& parname) const override {
        if (sendToParent(parname)) {
            return m_parent.has_preset_value(parname);
        } else {
            return consuming_broker::has_preset_value(parname);
        }
    }

    cci_value get_preset_cci_value(const std::string& parname) const override {
        if (sendToParent(parname)) {
            return m_parent.get_preset_cci_value(parname);
        } else {
            return consuming_broker::get_preset_cci_value(parname);
        }
    }

    void lock_preset_value(const std::string& parname) override {
        if (sendToParent(parname)) {
            return m_parent.lock_preset_value(parname);
        } else {
            return consuming_broker::lock_preset_value(parname);
        }
    }

    cci_value get_cci_value(const std::string& parname, const cci::cci_originator& originator =
                                                            cci::cci_originator()) const override {
        if (sendToParent(parname)) {
            return m_parent.get_cci_value(parname, originator);
        } else {
            return consuming_broker::get_cci_value(parname, originator);
        }
    }

    void add_param(cci_param_if* par) override {
        if (sendToParent(par->name())) {
            return m_parent.add_param(par);
        } else {
            auto iter = m_unignored.find(par->name());
            if (iter != m_unignored.end()) {
                m_unignored.erase(iter);
            }
            return consuming_broker::add_param(par);
        }
    }

    void remove_param(cci_param_if* par) override {
        if (sendToParent(par->name())) {
            return m_parent.remove_param(par);
        } else {
            m_unignored.insert(par->name());
            return consuming_broker::remove_param(par);
        }
    }

    // Upstream consuming broker fails to pass get_unconsumed_preset_value to parent
    std::vector<cci_name_value_pair> get_unconsumed_preset_values() const override {
        std::vector<cci_name_value_pair> r;
        for (auto u : m_unignored) {
            auto p = get_preset_cci_value(u);
            r.push_back(std::make_pair(u, p));
        }
        if (has_parent) {
            std::vector<cci_name_value_pair> p = m_parent.get_unconsumed_preset_values();
            r.insert(r.end(), p.begin(), p.end());
        }
        return r;
    }

    // Upstream consuming broker fails to pass ignore_unconsumed_preset_values
    void ignore_unconsumed_preset_values(const cci_preset_value_predicate& pred) override {
        if (has_parent) {
            m_parent.ignore_unconsumed_preset_values(pred);
        }
        for (auto u = m_unignored.begin(); u != m_unignored.end();) {
            if (pred(make_pair(*u, cci_value(0)))) {
                u = m_unignored.erase(u);
            } else {
                ++u;
            }
        }
    }

    cci_preset_value_range get_unconsumed_preset_values(
        const cci_preset_value_predicate& pred) const override {
        return cci_preset_value_range(pred, ConfigurableBroker::get_unconsumed_preset_values());
    }

    // Functions below here require an orriginator to be passed to the local
    // method variant.

    void set_preset_cci_value(const std::string& parname, const cci_value& cci_value,
                              const cci_originator& originator) override {
        if (sendToParent(parname)) {
            return m_parent.set_preset_cci_value(parname, cci_value, originator);
        } else {
            m_unignored.insert(parname);
            return consuming_broker::set_preset_cci_value(parname, cci_value, originator);
        }
    }
    cci_param_untyped_handle get_param_handle(const std::string& parname,
                                              const cci_originator& originator) const override {
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

    std::vector<cci_param_untyped_handle> get_param_handles(
        const cci_originator& originator) const override {
        if (has_parent) {
            std::vector<cci_param_untyped_handle> p_param_handles = m_parent.get_param_handles();
            std::vector<cci_param_untyped_handle>
                param_handles = consuming_broker::get_param_handles(originator);
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

    bool is_global_broker() const override { return (!has_parent); }
};
} // namespace gs

template <>
struct cci::cci_value_converter<gs::ConfigurableBroker*> {
    typedef gs::ConfigurableBroker* type;
    static bool pack(cci_value::reference dst, type const& src) {
        dst.set_string(src->name());
        return true;
    }
    static bool unpack(type& dst, cci_value::const_reference src) { return false; }
};

#endif // CCIUTILS_H
