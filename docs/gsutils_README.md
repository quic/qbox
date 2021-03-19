# namespace `gs`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[``ConfigurableBroker``](#classgs_1_1_configurable_broker)    | 

# class `ConfigurableBroker` {#classgs_1_1_configurable_broker}

```
class ConfigurableBroker
  : public cci_utils::consuming_broker
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public std::set< std::string > expose` | 
`public cci_param< std::string > conf_file` | 
`public inline  ConfigurableBroker(bool load_conf_file)` | 
`public inline  ConfigurableBroker(std::initializer_list< cci_name_value_pair > list,bool load_conf_file)` | 
`public inline  ConfigurableBroker(const int argc,const char *const * argv,std::initializer_list< cci_name_value_pair > list,bool load_conf_file)` | 
`public inline std::string relname(const std::string & n) const` | 
`public inline std::string relname(const char * n) const` | 
`public inline  ~ConfigurableBroker()` | 
`public inline cci_originator get_value_origin(const std::string & parname) const` | 
`public inline bool has_preset_value(const std::string & parname) const` | 
`public inline cci_value get_preset_cci_value(const std::string & parname) const` | 
`public inline void lock_preset_value(const std::string & parname)` | 
`public inline cci_value get_cci_value(const std::string & parname) const` | 
`public inline void add_param(cci_param_if * par)` | 
`public inline void remove_param(cci_param_if * par)` | 
`public inline void set_preset_cci_value(const std::string & parname,const cci_value & cci_value,const cci_originator & originator)` | 
`public inline cci_param_untyped_handle get_param_handle(const std::string & parname,const cci_originator & originator) const` | 
`public inline std::vector< cci_param_untyped_handle > get_param_handles(const cci_originator & originator) const` | 
`public inline bool is_global_broker() const` | 

## Members

### `public std::set< std::string > expose` {#classgs_1_1_configurable_broker_1ab847c4a1ac69512497f034158de9c599}





### `public cci_param< std::string > conf_file` {#classgs_1_1_configurable_broker_1ad9c2cd434573b79e7554735775ae3921}





### `public inline  ConfigurableBroker(bool load_conf_file)` {#classgs_1_1_configurable_broker_1a1d6c33577b1c796f3483eaa7bcf8c9a6}





### `public inline  ConfigurableBroker(std::initializer_list< cci_name_value_pair > list,bool load_conf_file)` {#classgs_1_1_configurable_broker_1ae751d6a90ebf1ff458b4093590a953b0}





### `public inline  ConfigurableBroker(const int argc,const char *const * argv,std::initializer_list< cci_name_value_pair > list,bool load_conf_file)` {#classgs_1_1_configurable_broker_1a8f78844d52e50c773a759849e2046162}





### `public inline std::string relname(const std::string & n) const` {#classgs_1_1_configurable_broker_1a43b04dd8f2b564255577a8023477b4d9}





### `public inline std::string relname(const char * n) const` {#classgs_1_1_configurable_broker_1a98f9b79db5cae5f6a2bf723bf6d52fc2}





### `public inline  ~ConfigurableBroker()` {#classgs_1_1_configurable_broker_1abbe91a6d6beb892f91cf9b1f166291cd}





### `public inline cci_originator get_value_origin(const std::string & parname) const` {#classgs_1_1_configurable_broker_1a8e756e5cb8fc1794014b3caa4fbe21cb}





### `public inline bool has_preset_value(const std::string & parname) const` {#classgs_1_1_configurable_broker_1a30525fb0431626a90472bfb971f745c8}





### `public inline cci_value get_preset_cci_value(const std::string & parname) const` {#classgs_1_1_configurable_broker_1a1cfd581be4b242fac6c0472408007bd9}





### `public inline void lock_preset_value(const std::string & parname)` {#classgs_1_1_configurable_broker_1ad7b74b7e301488fa534843f5a3d4a33d}





### `public inline cci_value get_cci_value(const std::string & parname) const` {#classgs_1_1_configurable_broker_1a9ea1b9d923a69ce3fe5d233723558dbd}





### `public inline void add_param(cci_param_if * par)` {#classgs_1_1_configurable_broker_1ab8a0108ff8d3e7c558771b60f3bb4997}





### `public inline void remove_param(cci_param_if * par)` {#classgs_1_1_configurable_broker_1a990dac3724ef12c044a95cd7a7e2a76c}





### `public inline void set_preset_cci_value(const std::string & parname,const cci_value & cci_value,const cci_originator & originator)` {#classgs_1_1_configurable_broker_1aec092ae7ecbe1b208b6a5f10ae4d30ea}





### `public inline cci_param_untyped_handle get_param_handle(const std::string & parname,const cci_originator & originator) const` {#classgs_1_1_configurable_broker_1a2eb4655b594909ad561d3a86c57dfb75}





### `public inline std::vector< cci_param_untyped_handle > get_param_handles(const cci_originator & originator) const` {#classgs_1_1_configurable_broker_1a4bb75b1601f2afcb4ae103aa3d594054}





### `public inline bool is_global_broker() const` {#classgs_1_1_configurable_broker_1ad58965b2d086cdf8832004ada345b130}






# namespace `cci`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# namespace `cci_utils`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# namespace `sc_core`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# class `LuaFile_Tool` {#class_lua_file___tool}

```
class LuaFile_Tool
  : public sc_core::sc_module
```  

Tool which reads a Lua configuration file and sets parameters.

Lua Config File Tool which reads a configuration file and uses the Tool_GCnf_Api to set the parameters during intitialize-mode.

One instance can be used to read and configure several lua config files.

The usage of this Tool:

* instantiate one object


* call config(filename)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  LuaFile_Tool(sc_core::sc_module_name name,std::string _orig_name)` | Constructor.
`public inline int config(const char * config_file)` | Makes the configuration.
`public inline void parseCommandLine(const int argc,const char *const * argv)` | Parses the command line and extracts the luafile option.
`protected inline void parseCommandLineWithGetOpt(const int argc,const char *const * argv)` | Parses the command line with getopt and extracts the luafile option.

## Members

### `public inline  LuaFile_Tool(sc_core::sc_module_name name,std::string _orig_name)` {#class_lua_file___tool_1af13bf98b2af8cadff4d477b2dc352c25}

Constructor.



### `public inline int config(const char * config_file)` {#class_lua_file___tool_1aad3cb644c9a1a988a75efc532e978196}

Makes the configuration.

Configure parameters from a lua file.

May be called several times with several configuration files

Example usage: 
```cpp
int sc_main(int argc, char *argv[]) {
  LuaFile_Tool luareader;
  luareader.config("file.lua");
  luareader.config("other_file.lua");
}
```

### `public inline void parseCommandLine(const int argc,const char *const * argv)` {#class_lua_file___tool_1a0d1d6a68571fd4092b2b51e033fa8454}

Parses the command line and extracts the luafile option.

Throws a CommandLineException.


#### Parameters
* `argc` The argc of main(...). 


* `argv` The argv of main(...).

### `protected inline void parseCommandLineWithGetOpt(const int argc,const char *const * argv)` {#class_lua_file___tool_1a85c28e769318fe7bbff45c5d46c91962}

Parses the command line with getopt and extracts the luafile option.

Throws a CommandLineException.


#### Parameters
* `argc` The argc of main(...). 


* `argv` The argv of main(...).


