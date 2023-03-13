

[//]: # DONT EDIT THIS FILE


[//]: # (SECTION 0)
## The GreenSocs SystemC Generic MAC library.

This includes simple ethernet MAC's. The components are "Loosely timed" only. 

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

## LIBGSSYNC

The GreenSocs Synchronization library provides a number of different policies for synchronizing between an external simulator (typically QEMU) and SystemC.

These are based on a proposed standard means to handle the SystemC simulator. This library provides a backwards compatibility layer, but the patched version of SystemC will perform better.

## LIBGSUTILS

The GreenSocs basic utilities library contains utility functions for CCI, simple logging and test functions.
It also includes some basic tlm port types

[//]: # (SECTION 10)


[//]: # (SECTION 10 AUTOADDED)

## Information about building and using the libgssync library
The libgssync library depends on the libraries : base-components, libgsutils, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.
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
## How to use GreenSocs SystemC Generic MAC library.
### SystemC Code
Needs at lease 0x8000 memory space. Initialize backend:
```
m_dwmac.set_backend(new NetworkBackendTap("dwmac-backend", "qbox0"));
```
### Linux Kernel
Add driver
```
CONFIG_DWMAC_GENERIC=y
Location:
  │     -> Device Drivers
  │       -> Network device support (NETDEVICES [=y])
  │         -> Ethernet driver support (ETHERNET [=y])
  │           -> STMicroelectronics devices (NET_VENDOR_STMICRO [=y])
  |             -> STMicroelectronics Multi-Gigabit Ethernet driver (STMMAC_ETH [=y])
  │               -> STMMAC Platform bus support (STMMAC_PLATFORM [=y])
```
Add device tree entry
```
gmac0: ethernet@200000 {
			compatible = "snps,dwmac";
			reg = <register space>;
			interrupt-parent = <&interrupt_parent>;
			interrupts = <interrupt_nr>;
			interrupt-names = "macirq";
			phy-mode = "gmii";
        };
```
### Builtroot
Enable sshd
```
BR2_PACKAGE_OPENSSH=y
BR2_PACKAGE_OPENSSH_SERVER=y
BR2_PACKAGE_OPENSSH_CLIENT=y
```

Enable root password. Doesn't work without it:
```
BR2_TARGET_GENERIC_ROOT_PASSWD=greensocs
```

To permit root login via ssh add
```
PermitRootLogin yes
```
to /etc/ssh/sshd_config

Configure network interface. Add this to /etc/network/interfaces
```
auto eth0
iface eth0 inet static
    address 192.168.0.97
    netmask 255.255.255.0
```
### Host
Start TUN interface
```
sudo ip tuntap add dev qbox0 mode tap user $(whoami)
sudo ip addr add 192.168.0.254/24 dev qbox0
sudo ip link set dev qbox0 up
```
After starting vp, log in via ssh
```
ssh root@192.168.0.97
```

[//]: # (SECTION 50 AUTOADDED)

## Functionality of the synchronization library
In addition the library contains utilities such as an thread safe event (async_event) and a real time speed limited for SystemC.

### Suspend/Unsuspend interface

This patch adds four new basic functions to SystemC:

```
void sc_suspend_all(sc_simcontext* csc= sc_get_curr_simcontext())
void sc_unsuspend_all(sc_simcontext* csc= sc_get_curr_simcontext())
void sc_unsuspendable()
void sc_suspendable()
```

**suspend_all/unsuspend_all :**
This pair of functions requests the kernel to ‘atomically suspend’ all processes (using the same semantics as the thread suspend() call). This is atomic in that the kernel will only suspend all the processes together, such that they can be suspended and unsuspended without any side effects. Calling suspend_all(), and subsiquently calling unsuspend_all() will have no effect on the suspended status of an individual process. 
A process may call suspend_all() followed by unsuspend_all, the calls should be ‘paired’, (multiple calls to either suspend_all() or unsuspend_all() will be ignored).
Outside of the context of a process, it is the programmers responsibility to ensure that the calls are paired.
As a consequence, multiple calls to suspend_all() may be made (within separate process, or from within sc_main). So long as there have been more calls to suspend_all() than to unsuspend_all(), the kernel will suspend all processes.

_[note, this patch set does not add convenience functions, including those to find out if suspension has happened, these are expected to be layered ontop]_

**unsusbendable()/suspendable():**
This pair of functions provides an ‘opt-out’ for specific process to the suspend_all(). The consequence is that if there is a process that has opted out, the kernel will not be able to suspend_all (as it would no longer be atomic). 
These functions can only be called from within a process.
A process should only call suspendable/unsuspendable in pairs (multiple calls to either will be ignored).
_Note that the default is that a process is marked as suspendable._


**Use cases:**
_1 : Save and Restore_
For Save and Restore, the expectation is that when a save is requested, ‘suspend_all’ will be called. If there are models that are in an unsuspendable state, the entire simulation will be allowed to continue until such a time that there are no unsuspendable processes.

_2 : External sync_
When an external model injects events into a SystemC model (for instance, using an ‘async_request_update()’), time can drift between the two simulators. In order to maintain time, SystemC can be prevented from advancing by calling suspend_all(). If there are process in an unsuspendable state (for instance, processing on behalf of the external model), then the simulation will be allowed to continue. 
NOTE, an event injected into the kernel by an async_request_update will cause the kernel to execute the associated update() function (leaving the suspended state). The update function should arrange to mark any processes that it requires as unsuspendable before the end of the current delta cycle, to ensure that they are scheduled.

## List of options of sync policy parameter

The libgssync library allows you to set several values to the `p_sync_policy` parameter :
- `tlm2`
- `multithread`
- `multithread-quantum`
- `multithread-rolling`
- `multithread-unconstrained`

By default the parameter is set to `multithread-quantum`.
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


[//]: # (PROCESSED BY doc_merge.pl)
