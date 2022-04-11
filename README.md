

[//]: # DONT EDIT THIS FILE


[//]: # (SECTION 0)
## The GreenSocs SystemC simple components library.

This includes simple models such as routers, memories and exclusive monitor. The components are "Loosely timed" only. They support DMI where appropriate, and make use of CCI for configuration.

It also has several unit tests for memory, router and exclusive monitor.

[//]: # (SECTION 0 AUTOADDED)


[//]: # (SECTION 1)


[//]: # (SECTION 1 AUTOADDED)

# GreenSocs Build and make system

# How to build
> 
> This project may be built using cmake
> ```bash
> cmake -B build;pushd build; make -j; popd
> ```
> 
cmake may ask for your git.greensocs.com credentials (see below for advice about passwords)

## cmake version
cmake version 3.14 or newer is required. This can be downloaded and used as follows
```bash
 curl -L https://github.com/Kitware/CMake/releases/download/v3.20.0-rc4/cmake-3.20.0-rc4-linux-x86_64.tar.gz | tar -zxf -
 ./cmake-3.20.0-rc4-linux-x86_64/bin/cmake
```
 
## details

This project uses CPM https://github.com/cpm-cmake/CPM.cmake in order to find, and/or download missing components. In order to find locally installed SystemC, you may use the standards SystemC environment variables: `SYSTEMC_HOME` and `CCI_HOME`.
CPM will use the standard CMAKE `find_package` mechanism to find installed packages https://cmake.org/cmake/help/latest/command/find_package.html
To specify a specific package location use `<package>_ROOT`
CPM will also search along the CMAKE_MODULE_PATH

Sometimes it is convenient to have your own sources used, in this case, use the `CPM_<package>_SOURCE_DIR`.
Hence you may wish to use your own copy of SystemC CCI 
```bash
cmake -B build -DCPM_SystemCCCI_SOURCE=/path/to/your/cci/source`
```

It may also be convenient to have all the source files downloaded, you may do this by running 
```bash
cmake -B build -DCPM_SOURCE_CACHE=`pwd`/Packages
```
This will populate the directory `Packages` Note that the cmake file system will automatically use the directory called `Packages` as source, if it exists.

NB, CMake holds a cache of compiled modules in ~/.cmake/ Sometimes this can confuse builds. If you seem to be picking up the wrong version of a module, then it may be in this cache. It is perfectly safe to delete it.

### Common CMake options
`CMAKE_INSTALL_PREFIX` : Install directory for the package and binaries.
`CMAKE_BUILD_TYPE`     : DEBUG or RELEASE

By default the value of the `CMAKE_INSTALL_PREFIX` variable is the `install/` directory located in the top level directory.

The library assumes the use of C++14, and is compatible with SystemC versions from SystemC 2.3.1a.

For a reference docker please use the following script from the top level of the Virtual Platform:
```bash
curl --header 'PRIVATE-TOKEN: W1Z9U8S_5BUEX1_Y29iS' 'https://git.greensocs.com/api/v4/projects/65/repository/files/docker_vp.sh/raw?ref=master' -o docker_vp.sh
chmod +x ./docker_vp.sh
./docker_vp.sh
> cmake -B build;cd build; make -j
```

### passwords for git.greensocs.com
To avoid using passwords for git.greensocs.com please add a ssh key to your git account. You may also use a key-chain manager. As a last resort, the following script will populate ~/.git-credentials  with your username and password (in plain text)
```bash
git config --global credential.helper store
```

## More documentation

More documentation, including doxygen generated API documentation can be found in the `/docs` directory.
## LIBGSUTILS

The GreenSocs basic utilities library contains utility functions for CCI, simple logging and test functions.
It also includes some basic tlm port types

[//]: # (SECTION 10)
## Information about building and using the base-components library
The base-components library depends on the libraries : Libgsutls, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

[//]: # (SECTION 10 AUTOADDED)

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
## The GreenSocs component library loader
The loader components exposes a single initiator socket (`initiator_socket`) which should be bound to the bus fabric through which it's intended to load memory components.
The loader can be confgured to load the following:
 * elf files\
Configure parameter: `elf_file`\
function: `void elf_load(const std::string& path)`\
 _Note, no address or offset is possible, as they are built into the elf file. If this is specified within the context of a memory, the addresses will be offset within the memory, this is probably NOT what is desired._

 * bin_file\
  Configure parameter `bin_file`\
  options: `address` (absolute address), or `offset` (relative address)\
  function: `void file_load(std::string filename, uint64_t addr)`

 * csv_file (32 bit data)\
 Configure parameter `csv_file`\
 options: `address` (absolute address), or `offset` (relative address)\
        `addr_str` : header in the CSV file for the column used for addresses\
        `value_str`: header in the CSV file for the column used for values\
        `byte_swap`: whether bytes should be swapped\
  function: `void csv_load(std::string filename, uint64_t offset, std::string addr_str std::string value_str, bool byte_swap)`

* param\
Configurable parameter `param`\
A configuration paramter that must be of type `std::string` is loaded into memory. The parameter must be realized such that a typed handled can be obtained.\
options: `address` (absolute address), or `offset` (relative address)\
function: `void str_load(std::string data, uint64_t addr)`

* data (32 bit data)\
Configure parameter `data`\
options: `byte_swap`: whether bytes should be swapped\
The data parameter is expected to be an array with index's numbered. The indexes will be used to address the memory.\
No function access is provided.

* ptr\
No config parameter is provided.\
function `void ptr_load(uint8_t *data, uint64_t addr, uint64_t len)`

as an example, a configuration may look like
`loader = {{elf_file="my_elf.elf"}, {data={0x1,0x2}, address = 0x30}}`

## The GreenSocs component library memory
The memory component allows you to add memory when creating an object of type `Memory("name")`.
The memory should be bound with it's tarket socket:
`tlm_utils::simple_target_socket<Memory> socket`

The memory accepts the following configuration options:
`read_only`: Read Only Memory (default false)\
`dmi_allow`: DMI allowed (default true)\
`verbose`: Switch on verbose logging (default false)\
`latency`: Latency reported for DMI access (default 10 NS)\
`map_file`: file used to map this memory (such that it can be preserved through runs) (default,none)

The memory's size is set by the address space allocated to it on it's target socket.
An optional size can be provided to the constructor.
The size set by the configurator will take precedence. The size given to the constructor will take precedence over that set on the target socket at bind time.

The memory has a 'loader' built in. configuration can be applied to `memory_name.load`. In this case, all addresses are assumed to be offsets.


## The GreenSocs component library router
The  router is a simple device, the expectation is that initiators and targets are directly bound to the router's `target_socket` and `initiator_socket` directly (both are multi-sockets).
The router will use CCI param's to discover the addresses and size of target devices as follows:
 * `<target_name>.<socket_name>.address`
 * `<target_name>.<socket_name>.size`
 * `<target_name>.<socket_name>.relative_addresses`

A default tlm2 "simple target socket" will have the name `simple_target_socket_0` by default (this can be changed in the target).
The `relative_addresses` flag is a boolean - targets which opt to have the router mask their address will receive addresses based from the IP base "address". Otherwise they will receive full addresses. The defaut is to receive relative addresses.

The router also offers `add_target(socket, base_address, size)` as a convenience, this will set appropriate param's (if they are not already set), and will set `relative_addresses` to be `true`.

Likewise the convenience function `add_initiator(socket)` allows multiple initiators to be connected to the router. Both `add_target` and `add_initiator` take care of binding.

__NB Routing is perfromed in _BIND_ order. In other words, overlapping addresses are allowed, and the first to match (in bind order) will be used. This allows 'fallback' routing.__

Also note that the router will add an extension called gs::PathIDExtension. This extension holds a std::vector of port index's (collectively a unique 'ID'). 

The ID is meant to be composed by all the routers on the path that
support this extension. This ID field can be used (for instance) to ascertain a unique ID for the issuing initiator.

The ID extension is held in a (thread safe) pool. Thread safety can be switched of in the code.

[//]: # (SECTION 50 AUTOADDED)

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
## The GreenSocs component Tests

It is possible to test the correct functioning of the different components with the tests that are proposed.

Once you have compiled your library, you will have a tests folder in your construction directory.

In this test folder you will find several folders, each corresponding to a component and each containing an executable.

You can run the executable with : 
```bash
./build/tests/<name_of_component>/<name_of_component>-tests
```

If you want a more general way to check the correct functioning of the components you can run all the tests at the same time with :
```bash
make test
```

[//]: # (PROCESSED BY doc_merge.pl)
