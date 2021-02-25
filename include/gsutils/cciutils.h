/*
 *  Copyright (C) 2020  GreenSocs
 *
 */

#ifndef CCIUTILS_H
#define CCIUTILS_H

#include <iostream>
#include <list>

#include <systemc>
#include <tlm>
#include <cci_configuration>

/*********************
 * CCI Convenience
 *********************/

namespace gs 
{
    /* return the leaf name of the given module name */
    static std::string sc_cci_leaf_name(std::string name)
    {
        return name.substr(name.find_last_of(".")+1);
    }

    /* return a list of children from the given module name, can be used inside
     * or outside the heirarchy */
    static std::list<std::string> sc_cci_children(sc_core::sc_module_name name)
    {
        cci::cci_broker_handle m_broker = (sc_core::sc_get_current_object())?cci::cci_get_broker():cci::cci_get_global_broker(cci::cci_originator("gs__sc_cci_children"));
        std::list<std::string> children;
        int l=strlen(name)+1;
        auto uncon = m_broker.get_unconsumed_preset_values(
            [&name]( const std::pair<std::string,cci::cci_value>& iv )
            { return iv.first.find( std::string(name) + "." ) == 0; });
        for( auto p : uncon ) {
            children.push_back(p.first.substr(l,p.first.find(".",l)-l));
        }
        children.sort();
        children.unique();
        return children;
    }
}

#endif // CCIUTILS_H
