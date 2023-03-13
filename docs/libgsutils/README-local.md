

[//]: # (SECTION 0)
## LIBGSUTILS

The GreenSocs basic utilities library contains utility functions for CCI, simple logging and test functions.
It also includes some basic tlm port types

[//]: # (SECTION 10)
Information about building and using the libgsutils library
-----------------------------------------------------------

The libgsutils library depends on the libraries : SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

The GreenSocs CCI libraries allows two options for setting configuration parameters

> `--gs_luafile <FILE.lua>` this option will read the lua file to set parameters. 

> `--param path.to.param=<value>` this option will allow individual parameters to be set.

NOTE, order is important, the last option on the command line to set a parameter will take preference.

This library includes a Configurable Broker (gs::ConfigurableBroker) which provides additional functionality. Each broker can be configured separately, and has a parameter itself for the configuration file to read. This is `lua_file`. Hence

> `--param path.to.module.lua_file="\"/host/path/to/lua/file\""`

Note that a string parameter must be quoted.

The lua file read by the ConfigurableBroker has relative paths - this means that in the example above the `path.to.module` portion of the absolute path should not appear in the (local) configuration file. (Hence changes in the hierarchy will not need changes to the configuration file).

## Using yaml for configuration
If you would prefer to use yaml as a configuration language, `lyaml` provides a link. This can be downloaded from https://github.com/gvvaughan/lyaml

The following lua code will load "conf.yaml".

```
local lyaml   = require "lyaml"

function readAll(file)
    local f = assert(io.open(file, "rb"))
    local content = f:read("*all")
    f:close()
    return content
end

print "Loading conf.yaml"
yamldata=readAll("conf.yaml")
ytab=lyaml.load(yamldata)
for k,v in pairs(ytab) do
    _G[k]=v
end
yamldata=nil
ytab=nil
```

[//]: # (SECTION 50)
## Using the ConfigurableBroker

The broker will self register in the SystemC CCI hierarchy. All brokers have a parameter `lua_file` which will be read and used to configure parameters held within the broker. This file is read at the *local* level, and paths are *relative* to the location where the ConfigurableBroker is instanced.

These brokers can be used as global brokers.

The `gs::ConfigurableBroker` can be instanced in 3 ways:
1. `ConfigurableBroker()`
    This will instance a 'Private broker' and will hide **ALL** parameters held within this broker. 
    
    A local `lua_file` can be read and will set parameters in the private broker. This can be prevented by passing 'false' as a construction parameter (`ConfigurableBroker(false)`).

2.  `ConfigurableBroker({{"key1","value1"},{"key2","value2")...})`
    This will instance a broker that sets and hides the listed keys. All other keys are passed through (exported). Hence the broker is 'invisible' for parameters that are not listed. This is specifically useful for structural parameters.
    
    It is also possible to instance a 'pass through' broker using `ConfigurationBroker({})`. This is useful to provide a *local* configuration broker than can, for instance, read a local configuration file.

    A local `lua_file` can be read and will set parameters in the private broker (exported or not). This can be prevented by passing 'false' as a construction parameter (`ConfigurableBroker(false)`). The `lua_file` will be read **AFTER** the construction key-value list and hence can be used to over-right default values in the code.

3.  `ConfigurableBroker(argc, argv)`
    This will instance a broker that is typically a global broker. The argc/argv values should come from the command line. The command line will be parsed to find:

    > `-p, --param path.to.param=<value>` this option will allow individual parameters to be set.
    
    > `-l, --gs_luafile <FILE.lua>` this option will read the lua file to set parameters. Similar functionality can be achieved using --param lua_file=\"<FILE.lua>\".

    A ``{{key,value}}`` list can also be provided, otherwise it is assumed to be empty. Such a list will set parameter values within this broker. These values will be read and used **BEFORE** the command line is read.

    Finally **AFTER** the command line is read, if the `lua_file` parameter has been set, the configuration file that it indicates will also be read. This can be prevented by passing 'false' as a construction parameter (`ConfigurableBroker(argc, argv, false)`). The `lua_file` will be read **AFTER** the construction key-value list, and after the command like, so it can be used to over-right default values in either.

## Print out the available params

It is possible to display the list of available cci parameters with the `-h` option when launching the virtual platform.

CAUTION:

This will only print the parameters at the begining of simulation.

[//]: # (SECTION 100)
## The GreenSocs utils Tests

Tests are available for you to check that the library is working properly.

To do this, you must run the following command in the build directoty `build/`:
```bash
make test
```

###  marking values as consumed
Use cci_get to both get a CCI preset value and mark the param as 'consumed':
 `` template <typename T> T cci_get(std::string name) ``
