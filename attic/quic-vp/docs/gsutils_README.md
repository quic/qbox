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
`public inline  ConfigurableBroker(const std::string & name,bool load_conf_file)` | 
`public inline  ConfigurableBroker(bool load_conf_file)` | 
`public inline  ConfigurableBroker(std::initializer_list< cci_name_value_pair > list,bool load_conf_file)` | 
`public inline  ConfigurableBroker(const int argc,const char *const * argv,std::initializer_list< cci_name_value_pair > list,bool load_conf_file)` | 
`public inline std::string relname(const std::string & n) const` | 
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





### `public inline  ConfigurableBroker(const std::string & name,bool load_conf_file)` {#classgs_1_1_configurable_broker_1aaf4d1550f05194e1a457e3e3ce862bee}





### `public inline  ConfigurableBroker(bool load_conf_file)` {#classgs_1_1_configurable_broker_1a1e4af7ca42c30cd8835044935ed1ad63}





### `public inline  ConfigurableBroker(std::initializer_list< cci_name_value_pair > list,bool load_conf_file)` {#classgs_1_1_configurable_broker_1ae751d6a90ebf1ff458b4093590a953b0}





### `public inline  ConfigurableBroker(const int argc,const char *const * argv,std::initializer_list< cci_name_value_pair > list,bool load_conf_file)` {#classgs_1_1_configurable_broker_1a512c3c92bdf62b17373b215755f09447}





### `public inline std::string relname(const std::string & n) const` {#classgs_1_1_configurable_broker_1a43b04dd8f2b564255577a8023477b4d9}





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


# class `TargetSignalSocket` {#class_target_signal_socket}

```
class TargetSignalSocket
  : public sc_core::sc_export< sc_core::sc_signal_inout_if< T > >
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  TargetSignalSocket(const char * name)` | 
`public inline void register_value_changed_cb(const ValueChangedCallback & cb)` | 
`public inline const T & read() const` | 
`protected `[`TargetSignalSocketProxy`](#class_target_signal_socket_proxy)`< T > m_proxy` | 

## Members

### `public inline  TargetSignalSocket(const char * name)` {#class_target_signal_socket_1a59a76ce35efe3dd1380daa346f92bf78}





### `public inline void register_value_changed_cb(const ValueChangedCallback & cb)` {#class_target_signal_socket_1ade4ddae4ad08e6f652864a4d758b2735}





### `public inline const T & read() const` {#class_target_signal_socket_1ac0c213c8e85754d27fce8e426bcbc5ad}





### `protected `[`TargetSignalSocketProxy`](#class_target_signal_socket_proxy)`< T > m_proxy` {#class_target_signal_socket_1a4c31775bdf62925527a5f039384edcf4}






# class `TargetSignalSocketProxy` {#class_target_signal_socket_proxy}

```
class TargetSignalSocketProxy
  : public sc_core::sc_signal_inout_if< T >
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  TargetSignalSocketProxy(`[`TargetSignalSocket`](#class_target_signal_socket)`< T > & parent)` | 
`public inline void register_value_changed_cb(const ValueChangedCallback & cb)` | 
`public inline `[`TargetSignalSocket`](#class_target_signal_socket)`< T > & get_parent()` | 
`public inline virtual const sc_core::sc_event & value_changed_event() const` | 
`public inline virtual const T & read() const` | 
`public inline virtual const T & get_data_ref() const` | 
`public inline virtual bool event() const` | 
`public inline virtual void write(const T & val)` | 
`protected `[`TargetSignalSocket`](#class_target_signal_socket)`< T > & m_parent` | 
`protected T m_val` | 
`protected ValueChangedCallback m_cb` | 
`protected sc_core::sc_event m_ev` | 

## Members

### `public inline  TargetSignalSocketProxy(`[`TargetSignalSocket`](#class_target_signal_socket)`< T > & parent)` {#class_target_signal_socket_proxy_1a26112bfc896bd815a313d2e8a1684120}





### `public inline void register_value_changed_cb(const ValueChangedCallback & cb)` {#class_target_signal_socket_proxy_1a8f335f310bc84f145773df1b7570a8fa}





### `public inline `[`TargetSignalSocket`](#class_target_signal_socket)`< T > & get_parent()` {#class_target_signal_socket_proxy_1a64d9055e1e0d3cbd55c1c28adadd104f}





### `public inline virtual const sc_core::sc_event & value_changed_event() const` {#class_target_signal_socket_proxy_1a348aff27d5e4b8198034f61d186afbd5}





### `public inline virtual const T & read() const` {#class_target_signal_socket_proxy_1a0a4eb499d986bc28d7e9b9a1f519c965}





### `public inline virtual const T & get_data_ref() const` {#class_target_signal_socket_proxy_1af5f41fa626bf66ce54e102c4418ca62d}





### `public inline virtual bool event() const` {#class_target_signal_socket_proxy_1a36989c04c07521363770efddcbac1845}





### `public inline virtual void write(const T & val)` {#class_target_signal_socket_proxy_1a0879f42e84061aa22d6af08fe78e6a20}





### `protected `[`TargetSignalSocket`](#class_target_signal_socket)`< T > & m_parent` {#class_target_signal_socket_proxy_1af2db3d97adaf6a23434ea43443d22fe4}





### `protected T m_val` {#class_target_signal_socket_proxy_1a783fa8f426d21e764d6fa2a0aec8e23f}





### `protected ValueChangedCallback m_cb` {#class_target_signal_socket_proxy_1ac00e1a51df8097d85a04c77ac3ec5815}





### `protected sc_core::sc_event m_ev` {#class_target_signal_socket_proxy_1a151c72fe71e76ae57480305900aa00f3}






# class `TargetSignalSocketProxy< bool >` {#class_target_signal_socket_proxy_3_01bool_01_4}

```
class TargetSignalSocketProxy< bool >
  : public sc_core::sc_signal_inout_if< bool >
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  TargetSignalSocketProxy(`[`TargetSignalSocket`](#class_target_signal_socket)`< bool > & parent)` | 
`public inline void register_value_changed_cb(const ValueChangedCallback & cb)` | 
`public inline `[`TargetSignalSocket`](#class_target_signal_socket)`< bool > & get_parent()` | 
`public inline virtual const sc_core::sc_event & value_changed_event() const` | 
`public inline virtual const sc_core::sc_event & posedge_event() const` | 
`public inline virtual const sc_core::sc_event & negedge_event() const` | 
`public inline virtual const bool & read() const` | 
`public inline virtual const bool & get_data_ref() const` | 
`public inline virtual bool event() const` | 
`public inline virtual bool posedge() const` | 
`public inline virtual bool negedge() const` | 
`public inline virtual void write(const bool & val)` | 
`protected `[`TargetSignalSocket`](#class_target_signal_socket)`< bool > & m_parent` | 
`protected bool m_val` | 
`protected ValueChangedCallback m_cb` | 
`protected sc_core::sc_event m_ev` | 
`protected sc_core::sc_event m_posedge_ev` | 
`protected sc_core::sc_event m_negedge_ev` | 

## Members

### `public inline  TargetSignalSocketProxy(`[`TargetSignalSocket`](#class_target_signal_socket)`< bool > & parent)` {#class_target_signal_socket_proxy_3_01bool_01_4_1abf19cbee1f578b9a9b68c29ef06f2829}





### `public inline void register_value_changed_cb(const ValueChangedCallback & cb)` {#class_target_signal_socket_proxy_3_01bool_01_4_1a003c42cc6237bc8c75312f215a1f0513}





### `public inline `[`TargetSignalSocket`](#class_target_signal_socket)`< bool > & get_parent()` {#class_target_signal_socket_proxy_3_01bool_01_4_1a83b15c8522721ba18f58dc2f32fbb326}





### `public inline virtual const sc_core::sc_event & value_changed_event() const` {#class_target_signal_socket_proxy_3_01bool_01_4_1aac6ca6b0103cbfde5bbcf6ed66ce2365}





### `public inline virtual const sc_core::sc_event & posedge_event() const` {#class_target_signal_socket_proxy_3_01bool_01_4_1acbc0d6ab668362a09eb0e74f70de5dc4}





### `public inline virtual const sc_core::sc_event & negedge_event() const` {#class_target_signal_socket_proxy_3_01bool_01_4_1a227c333441d40f48f0f1be83d1334aac}





### `public inline virtual const bool & read() const` {#class_target_signal_socket_proxy_3_01bool_01_4_1a9130110eac809a68f42fbe7e25f92284}





### `public inline virtual const bool & get_data_ref() const` {#class_target_signal_socket_proxy_3_01bool_01_4_1ac9d4cada16410a421b1bb52f29f26361}





### `public inline virtual bool event() const` {#class_target_signal_socket_proxy_3_01bool_01_4_1a1da976d3d821cb426c8b2938e85ac17f}





### `public inline virtual bool posedge() const` {#class_target_signal_socket_proxy_3_01bool_01_4_1aef6264912c525b5e42d64f92957fe973}





### `public inline virtual bool negedge() const` {#class_target_signal_socket_proxy_3_01bool_01_4_1aa9597386c2926fb209038fed97bbf0d7}





### `public inline virtual void write(const bool & val)` {#class_target_signal_socket_proxy_3_01bool_01_4_1a570969d42d6fdac05a44257d0ac0b09d}





### `protected `[`TargetSignalSocket`](#class_target_signal_socket)`< bool > & m_parent` {#class_target_signal_socket_proxy_3_01bool_01_4_1a39bc8ccdb6115a77a59a2ace3ceec488}





### `protected bool m_val` {#class_target_signal_socket_proxy_3_01bool_01_4_1ad38be1e02819020bc16da54035b769f0}





### `protected ValueChangedCallback m_cb` {#class_target_signal_socket_proxy_3_01bool_01_4_1a05e8ee504bc86dab1af5c591cc6684e1}





### `protected sc_core::sc_event m_ev` {#class_target_signal_socket_proxy_3_01bool_01_4_1aa3d68f5956864b59a6bb02c10c21dc08}





### `protected sc_core::sc_event m_posedge_ev` {#class_target_signal_socket_proxy_3_01bool_01_4_1a0b6c3df6d680bc3937f77d921d662b50}





### `protected sc_core::sc_event m_negedge_ev` {#class_target_signal_socket_proxy_3_01bool_01_4_1ab60826103a37c833e874ce14d8c47976}






