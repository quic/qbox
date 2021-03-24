/*
 *  Copyright (C) 2020  GreenSocs
 *
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

#define DEBUG(name, msg)                                                       \
  std::cout << "@" << sc_core::sc_time_stamp() << " /"                         \
            << (unsigned)sc_core::sc_delta_count() << " (" << name             \
            << "): " << msg << std::endl

#define ENABLE_SHORT_COMMAND_LINE_OPTIONS // enables the short synonyms for the
                                          // gs_ options

// if this should use the unix getopt function for the parsing of the command
// line options or (if not) the boost program_options should be used (don't
// forget to link the lib boost_program_options!) default: do NOT define this!
// #define USE_GETOPT

#include <cci_configuration>
#include <iostream>
#include <string>

#define USE_GETOPT

#ifdef USE_GETOPT
#include <getopt.h>
#else
#include <boost/program_options.hpp>
#endif

#ifdef HAS_LUA
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#endif

#ifndef USE_GETOPT
namespace po = boost::program_options;
#endif

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

class LuaFile_Tool : public sc_core::sc_module {

  cci::cci_broker_handle m_broker; ///< CCI configuration handle
  std::string m_orig_name;

  std::string rel(std::string &n) const {
    if (m_orig_name.empty()) return n;
    else return (m_orig_name + n);
  }

public:
  /// Constructor
  LuaFile_Tool(sc_core::sc_module_name name, std::string _orig_name = "")
      : sc_core::sc_module(name), m_broker(cci::cci_get_broker()) {
    if (_orig_name.empty()) m_orig_name = "";
    else m_orig_name = _orig_name + ".";
  }

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
   */
  int config(const char *config_file) {
#ifndef HAS_LUA
    std::cerr << "Lua file specified, but no LUA support compiled in\n";
    exit(-1);
#else
    DEBUG(name(), "Read lua file '" << config_file << "'");

    // start Lua
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // load a script as the function "config_chunk"
    int error = luaL_loadfile(L, config_file);
    switch (error) {
    case 0:
      break;
    case LUA_ERRSYNTAX:
      fprintf(stderr, "Syntax error reading config file: %s\n", config_file);
      return 1;
    case LUA_ERRMEM:
      fprintf(stderr, "Error allocating memory to read config file: %s\n",
              config_file);
      return 1;
    case LUA_ERRFILE:
      fprintf(stderr, "Error opening/reading the config file: %s\n",
              config_file);
      return 1;
    default:
      fprintf(stderr, "Unknown error loading config file: %s\n", config_file);
      return 1;
    }
    lua_setglobal(L, "config_chunk");

    // little script to run the file
    const char *config_loader =
        "-- put some commands here to run before the user script\n"
        "config_chunk()";

    // run
    if (luaL_dostring(L, config_loader)) {
      fprintf(stderr, "%s\n", lua_tostring(L, -1));
      lua_pop(L, 1); /* pop error message from the stack */
    }

    // traverse the environment table setting global variables as parameters
    //      lua_getfield(L, LUA_GLOBALSINDEX, "_G");
    // getglobal should work for both lua 5.1 and 5.2
    lua_getglobal(L, "_G");
    error = setParamsFromLuaTable(L, lua_gettop(L));
    if (error < 0) {
      fprintf(stderr, "Error loading lua config file: %s\n", config_file);
      return error;
    }
    return 0;
#endif
  }

  /// Parses the command line and extracts the luafile option.
  /**
   * Throws a CommandLineException.
   *
   * @param argc The argc of main(...).
   * @param argv The argv of main(...).
   */
  void
  parseCommandLine(const int argc,
                   const char *const *argv) /* throw(CommandLineException) */ {
#ifdef USE_GETOPT
    parseCommandLineWithGetOpt(argc, argv);
#else
    parseCommandLineWithBoost(argc, argv);
#endif
  }

protected:
#ifndef USE_GETOPT
  /// Parses the command line with boost::program_options and extracts the
  /// luafile option.
  /**
   * Throws a CommandLineException.
   *
   * @param argc The argc of main(...).
   * @param argv The argv of main(...).
   */
  void parseCommandLineWithBoost(
      const int argc, const char *const *argv) /*throw(CommandLineException)*/ {
    DEBUG(name(), "Parse command line for --gs_luafile option ("
                      << argc << " arguments)");

    po::options_description desc("Allowed options");
#ifdef ENABLE_SHORT_COMMAND_LINE_OPTIONS
    desc.add_options()("help,h",
                       "  Command line usage for command line Config parser")(
        "gs_luafile,l", po::value<std::vector<std::string>>(),
        "<filename>   execute a Lua script and loads all the globals as "
        "parameters");
#else
    desc.add_options()("help",
                       "  Command line usage for command line Config parser")(
        "gs_luafile", po::value<std::vector<std::string>>(),
        "<filename>   execute a Lua script and loads all the globals as "
        "parameters");
#endif

    po::variables_map vm;
    // po::store(po::parse_command_line(argc, argv, desc), vm); // without
    // allowing unknown options
    po::store(po::command_line_parser(argc, const_cast<char **>(argv))
                  .options(desc)
                  .allow_unregistered()
                  .run(),
              vm); // allows unknown options

    if (vm.count("help")) {
      std::cout << "Command line usage for lua file command line parser:"
                << std::endl;
      std::cout << "  Usage: options_description [options]" << std::endl;
      std::cout << desc;
      return;
    }

    if (vm.count("gs_luafile")) {
      const std::vector<std::string> *vec =
          &vm["gs_luafile"].as<std::vector<std::string>>();
      for (unsigned int i = 0; i < vec->size(); i++) {
        DEBUG(name(), "Option gs_luafile with value " << vec->at(i).c_str());
        std::cout << "Lua file command line parser: parse option --gs_luafile "
                  << vec->at(i).c_str() << std::endl;
        config(vec->at(i).c_str());
      }
    }
  }

#else

  /// Parses the command line with getopt and extracts the luafile option.
  /**
   * Throws a CommandLineException.
   *
   * @param argc The argc of main(...).
   * @param argv The argv of main(...).
   */
  void parseCommandLineWithGetOpt(
      const int argc,
      const char *const *argv) /* throw(CommandLineException) */ {
    DEBUG(name(), "Parse command line for --gs_luafile option ("
                      << argc << " arguments)");

    // getopt permutes argv array, so copy it to argv_cp
    const int BUFFER_SIZE = 8192;
    char argv_buffer[BUFFER_SIZE];
    char *buffer_p = argv_buffer;
    char **argv_cp = new char *[argc];
    for (int i = 0; i < argc; i++) {
      size_t len = strlen(argv[i]) + 1; // count \0
      strcpy(buffer_p, argv[i]);
      argv_cp[i] = buffer_p;
      buffer_p += len;
    }

    // Check the rare case that BUFFER_SIZE is not enough
    if (buffer_p - argv_buffer > BUFFER_SIZE) {
      throw std::overflow_error("buffer overflow");
    }

    // configure getopt
    optind = 0; // reset of getopt
    opterr = 0; // avoid error message for not recognized option
#ifdef ENABLE_SHORT_COMMAND_LINE_OPTIONS
    static const char *optstring = "l:p:h";
#else
    static const char *optstring = "";
#endif
    static struct option long_options[] = {
        {"gs_luafile", 1, 0, 'l'}, // '--luafile filename'
        {"param", 1, 0, 'p'},      // --param foo.baa=10
        {"help", 0, 0, 'h'},       // '--help' = '-h'
        {0, 0, 0, 0}};

    while (1) {

      int c = getopt_long(argc, argv_cp, optstring, long_options, 0);
      if (c == EOF)
        break;

      switch (c) {

      case 'l': // -l and --gs_luafile
        DEBUG(name(), "Option --gs_luafile with value " << optarg);
        std::cout << "Lua file command line parser: parse option --gs_luafile "
                  << optarg << std::endl;
        config(optarg);
        break;

      case 'p': // -p and --param
      {
        std::stringstream ss(optarg);
        std::string key, value;
        if (!std::getline(ss, key, '=')) {
          std::cout << "parameter name not found!" << std::endl;
          exit(-1);
        }
        if (!std::getline(ss, value)) {
          std::cout << "parameter value not found!" << std::endl;
          exit(-1);
        }
        DEBUG(name(), "Setting param " << rel(key) << " to value " << value);
        m_broker.set_preset_cci_value(
            rel(key), cci::cci_value(cci::cci_value::from_json(value)));
        break;
      }
      case 'h': // -h and --help
        std::cout << "Lua file command line parser: parse option --help "
                  << std::endl;
        std::cout << "  Command line usage for lua file command line parser:"
                  << std::endl;
        std::cout << std::endl;
        std::cout << "     Possible Options/Arguments:" << std::endl;
        std::cout << std::endl;
        std::cout << "      --gs_luafile <filename>" << std::endl;
        std::cout << "        execute a Lua script and loads all the globals "
                     "as parameters"
                  << std::endl;
        std::cout << std::endl;
        std::cout << "      --param <param_name=value>" << std::endl;
        std::cout << "        set param name (foo.baa) to value" << std::endl;
        std::cout << std::endl;
        std::cout << "      --help" << std::endl;
        std::cout << "        this help" << std::endl;
        std::cout << std::endl;
        std::cout << std::endl;
        break;

      case '?':
      case ':':
        DEBUG(name(), "Option "
                          << c
                          << " not processed in lua file command line parser: "
                          << optopt);
        break;
      }
    }
  }
#endif

#ifdef HAS_LUA
  /// Traverse a Lua table setting global variables as parameters (recursive)
  /**
   * @param L Lua state
   * @param t Lua index
   * @param level (only for recursion) parameter name prefixed
   * @return number of elements if lua array or error if negative
   */
  int setParamsFromLuaTable(lua_State *L, int t, char *level = NULL) {
    /* start up */
    const int MAX_NAME_SIZE = 1000;
    static char static_key[MAX_NAME_SIZE];
    static char *key;
    static char value[100]; // used only to convert LUA_TNUMBER
    int should_inc_integer_index_count;
    int integer_index_count = 0;
    char *next_level;
    if (level == NULL) {
      key = static_key;
      level = key;
    }

    /* test for overflow (hopefully unlikelly, so test only after it happens,
     * for sanity) */
    if (level - static_key > MAX_NAME_SIZE) {
      static_key[MAX_NAME_SIZE - 1] = 0;
      fprintf(stderr,
              "FATAL Error: parameter name too big (bigger then %d): %s",
              MAX_NAME_SIZE, static_key);
    }

    /* is it really a table? */
    if (lua_type(L, t) != LUA_TTABLE) {
      fprintf(stderr, "Error: argument is not a table");
      return -1;
    }

    /* traverse table */
    lua_pushnil(L); /* first key */

    /* adjust t if relative index */
    if (t < 0)
      --t;

    while (lua_next(L, t) != 0) {

      /* reset flag */
      should_inc_integer_index_count = false;

      /* set the key */
      switch (lua_type(L, -2)) {

      case LUA_TNUMBER:
        // key must be integer values (ignore floating part)
        // also convert from 1-based to 0-based indexes (decrement 1)
        should_inc_integer_index_count = true;
#ifdef __MINGW32__
        next_level =
            level +
            __mingw_sprintf(level, "%lld", (long long)lua_tonumber(L, -2) - 1);
#else
        next_level =
            level + sprintf(level, "%lld", (long long)lua_tonumber(L, -2) - 1);
#endif
        break;

      case LUA_TSTRING:
        // avoid using stpcpy as it is not defined in MSVS, use strcpy+length
        // instead
        next_level = strcpy(level, lua_tostring(L, -2));
        next_level += strlen(level);
        break;

      default:
        fprintf(stderr, "Error loading lua file: invalid key");
        return -1;
      }

      /* set key value in the database */
      switch (lua_type(L, -1)) {

      case LUA_TNUMBER:
        // Avoid setting some Lua specific values as parameters
        if (strcmp(key, "math.huge") == 0 || strcmp(key, "math.pi") == 0 || 0) {
          if (GC_LUA_DEBUG)
            fprintf(stderr, "(%s) %s   (ignored because it's Lua specific)\n",
                    lua_typename(L, lua_type(L, -1)), key);
        } else {
          double num = lua_tonumber(L, -1);
          if ((uint64_t)num == num) {
            std::string keys = key;
            m_broker.set_preset_cci_value(rel(keys),
                                          cci::cci_value((uint64_t)num));
          } else if ((int64_t)num == num) {
            std::string keys = key;
            m_broker.set_preset_cci_value(rel(keys),
                                          cci::cci_value((int64_t)num));
          } else {
            std::string keys = key;
            m_broker.set_preset_cci_value(rel(keys), cci::cci_value(num));
          }

          if (GC_LUA_VERBOSE) {
            std::string keys = key;
            fprintf(stderr, "(SET %s) %s = %s\n",
                    lua_typename(L, lua_type(L, -1)), rel(keys).c_str(),
                    cci::cci_value(num).to_json().c_str());
          }
          if (should_inc_integer_index_count)
            ++integer_index_count;
        }
        break;

      case LUA_TBOOLEAN: {
        std::string keys = key;
        m_broker.set_preset_cci_value(
            rel(keys), cci::cci_value((bool)lua_toboolean(L, -1)));
        if (GC_LUA_VERBOSE)
          fprintf(stderr, "(SET %s) %s = %s\n",
                  lua_typename(L, lua_type(L, -1)), rel(keys).c_str(),
                  lua_toboolean(L, -1) ? "true" : "false");
        if (should_inc_integer_index_count)
          ++integer_index_count;
        break;
      }
      case LUA_TSTRING:
        // Avoid setting some Lua specific values as parameters
        if (strcmp(key, "_VERSION") == 0 || strcmp(key, "package.cpath") == 0 ||
            strcmp(key, "package.config") == 0 ||
            strcmp(key, "package.path") == 0 || 0) {
          if (GC_LUA_DEBUG)
            fprintf(stderr, "(%s) %s   (ignored because it's Lua specific)\n",
                    lua_typename(L, lua_type(L, -1)), key);
        } else {
          std::string keys = key;
          m_broker.set_preset_cci_value(
              rel(keys), cci::cci_value(std::string(lua_tostring(L, -1))));
          if (GC_LUA_VERBOSE)
            fprintf(stderr, "(SET %s) %s = %s\n",
                    lua_typename(L, lua_type(L, -1)), rel(keys).c_str(),
                    lua_tostring(L, -1));
          if (should_inc_integer_index_count)
            ++integer_index_count;
        }
        break;

      case LUA_TTABLE:
        // Avoid recursion on some tables
        if (strcmp(key, "_G") == 0 || strcmp(key, "package.loaded") == 0 ||
            strcmp(level, "__index") == 0) {
          if (GC_LUA_DEBUG)
            fprintf(stderr, "(%s) %s   (ignored to avoid recursion)\n",
                    lua_typename(L, lua_type(L, -1)), key);
        } else {
          if (GC_LUA_DEBUG)
            fprintf(stderr, "(table) %s\n", key);
          *next_level++ = '.';
          // CS
          // int int_index_count =
          setParamsFromLuaTable(L, -1, next_level);
          // CS
          // if (int_index_count > 0) {
          // sprintf(value, "%d", int_index_count);
          // mApi->setInitValue((std::string(key).substr(0, next_level-key) +
          // "init_size").c_str(), value); if (GC_LUA_VERBOSE) fprintf(stderr,
          // "(SET number) %s = %s\n", (std::string(key).substr(0,
          // next_level-key) + "init_size").c_str(), value);
          //}
        }
        break;

      case LUA_TFUNCTION:
      case LUA_TNIL:
      case LUA_TUSERDATA:
      case LUA_TTHREAD:
      case LUA_TLIGHTUSERDATA:
      default:
        // Ignore other types
        if (GC_LUA_DEBUG)
          fprintf(stderr, "(%s) %s\n", lua_typename(L, lua_type(L, -1)), key);
      }

      /* removes 'value'; keeps 'key' for next iteration */
      lua_pop(L, 1);
    }

    return integer_index_count;
  }
#endif
};

#endif
