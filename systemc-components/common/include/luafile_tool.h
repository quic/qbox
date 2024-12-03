/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __LUAFILE_TOOL_H__
#define __LUAFILE_TOOL_H__

// Set to true (or use -DGC_LUA_VERBOSE=true argument) to show the parameters
// set
#ifndef GC_LUA_VERBOSE
#define GC_LUA_VERBOSE false
#endif

// Set to true (or use -DGC_LUA_DEBUG=true argument) to show what was not set as
// a parameter
#ifndef GC_LUA_DEBUG
#define GC_LUA_DEBUG false
#endif

#include <scp/report.h>
#include <cci_configuration>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <getopt.h>
#include <memory>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int lua_cci_get_val(lua_State* L);
}

/// Tool which reads a Lua configuration file and sets parameters.
/**
 * Lua Config File Tool which reads a configuration file and uses the
 * Tool_GCnf_Api to set the parameters during intitialize-mode.
 *
 * One instance can be used to read and configure several lua config files.
 *
 * The usage of this Tool:
 * - instantiate one object
 * - call config(filename)
 */

namespace gs {
class LuaFile_Tool
{
    SCP_LOGGER();
    using key_val_args = std::vector<std::pair<std::string, std::string>>;

    std::string m_orig_name;

    std::string rel(std::string& n) const;

public:
    /// Constructor
    LuaFile_Tool(std::string _orig_name = "");

    /// Makes the configuration
    /**
     * Configure parameters from a lua file.
     *
     * May be called several times with several configuration files
     *
     * Example usage:
     * \code
     *    int sc_main(int argc, char *argv[]) {
     *      LuaFile_Tool luareader;
     *      luareader.config("file.lua");
     *      luareader.config("other_file.lua");
     *    }
     * \endcode
     * @param a_broker cci brocker handle
     * @param a_config_file lua config file
     * @param a_lua_globalvars global variables that needs to be set on the lua config
     * @param a_images_dir directory including software images.
     */
    int config(cci::cci_broker_handle a_broker, const char* a_config_file, char* a_images_dir = NULL);

protected:
    /// Traverse a Lua table setting global variables as parameters (recursive)
    /**
     * @param L Lua state
     * @param t Lua index
     * @param level (only for recursion) parameter name prefixed
     * @return number of elements if lua array or error if negative
     */
    int setParamsFromLuaTable(cci::cci_broker_handle a_broker, lua_State* L, int t, char* level = NULL);

    /// set global variables passed from command line arguments using -D | --define_luagvar
    /**
     * accepts only these data types: booleans, numbers and strings
     * @param L lua_State*
     * @param a_lua_globalvars std::vector of std::pairs of lua passed global variables.
     */
    void set_g_vars_from_cmdline(lua_State* L, const key_val_args& a_lua_globalvars);
};
} // namespace gs

#endif
