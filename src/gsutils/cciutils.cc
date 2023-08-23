//==============================================================================
//
// Copyright (c) 2023 Qualcomm Innovation Center, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above
//      copyright notice, this list of conditions and the following
//      disclaimer in the documentation and/or other materials provided
//      with the distribution.
//
//    * Neither the name Qualcomm Innovation Center nor the names of its
//      contributors may be used to endorse or promote products derived
//      from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//==============================================================================

#include "greensocs/gsutils/cciutils.h"

using namespace cci;

/**
 * @brief Helper function to find a sc_object by fully qualified name
 *  throws SC_ERROR if nothing found.
 * @param m     current parent object (use null to start from the top)
 * @param name  name being searched for
 * @return sc_core::sc_object* return object if found
 */
sc_core::sc_object* gs::find_sc_obj(sc_core::sc_object* m, std::string name, bool test)
{
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
                SCP_ERR("cciutils.find_sc_obj") << "Unable to find " << name << " in sc_object " << m->name();
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
std::string gs::sc_cci_leaf_name(std::string name) { return name.substr(name.find_last_of(".") + 1); }

/**
 * @brief return a list of 'unconsumed' children from the given module name, can be used inside
 * or outside the heirarchy
 *
 * @param name
 * @return std::list<std::string>
 */
std::list<std::string> gs::sc_cci_children(sc_core::sc_module_name name)
{
    cci_broker_handle m_broker = (sc_core::sc_get_current_object())
                                     ? cci_get_broker()
                                     : cci_get_global_broker(cci_originator("gs__sc_cci_children"));
    std::list<std::string> children;
    int l = strlen(name) + 1;
    auto uncon = m_broker.get_unconsumed_preset_values(
        [&name](const std::pair<std::string, cci_value>& iv) { return iv.first.find(std::string(name) + ".") == 0; });
    for (auto p : uncon) {
        children.push_back(p.first.substr(l, p.first.find(".", l) - l));
    }
    children.sort();
    children.unique();
    return children;
}

// #pragma GCC diagnostic ignored "-Wunused-function"
/**
 * @brief return a list of 'unconsumed' items in the module matching the base name given, plus '_'
 * and a numeric value typically this would be a list of items from an sc_vector Can be used inside
 * or outside the heirarchy
 *
 * @param module_name : name of the module
 * @param list_name : base name of the list (e.g. name of the sc_vector)
 * @return std::list<std::string>
 */
std::list<std::string> gs::sc_cci_list_items(sc_core::sc_module_name module_name, std::string list_name)
{
    cci_broker_handle m_broker = (sc_core::sc_get_current_object())
                                     ? cci_get_broker()
                                     : cci_get_global_broker(cci_originator("gs__sc_cci_children"));
    std::string name = std::string(module_name) + "." + list_name;
    std::regex search(name + "_[0-9]+(\\.|$)");
    std::list<std::string> children;
    int l = strlen(module_name) + 1;
    auto uncon = m_broker.get_unconsumed_preset_values(
        [&search](const std::pair<std::string, cci::cci_value>& iv) { return std::regex_search(iv.first, search); });
    for (auto p : uncon) {
        std::smatch match;
        std::regex_search(p.first, match, search);
        children.push_back(p.first.substr(l, (match.length() - l) - (match.str().back() == '.' ? 1 : 0)));
    }
    children.sort();
    children.unique();
    return children;
}

/* handle static list for CCI parameter pre-registration */
#include <greensocs/gsutils/module_factory_registery.h>

std::vector<std::function<cci::cci_param<gs::cci_constructor_vl>*()>>* gs::ModuleFactory::GetAvailableModuleList()
{
    static std::vector<std::function<cci::cci_param<gs::cci_constructor_vl>*()>> list;
    return &list;
}
