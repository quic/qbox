

[//]: # DONT EDIT THIS FILE


[//]: # (SECTION 0)
# Virtual Platform of ARM Cortex A53 and Hexagon for Qualcomm

## 1. Overview

This release contains an example of a virtual platform based on an ARM Cortex A53 and Hexagon DSP.
## 2. Requirements

You can build this release natively on Ubuntu 18.04.

Install dependencies:
```bash
apt update && apt upgrade -y
apt install -y make cmake g++ wget flex bison unzip python pkg-config libpixman-1-dev libglib2.0-dev libelf-dev
```

## 3. Fetch the qbox sources

If you have an SSH key:
```bash
cd $HOME
git@git.greensocs.com:customers/qualcomm/qualcomm_vp.git
```

otherwise:
```bash
cd $HOME
https://git.greensocs.com/customers/qualcomm/qualcomm_vp.git
```

this will extract the platform in $HOME/qualcomm_vp

## 4. Build the platform

```bash
cd $HOME/qualcomm_vp
mkdir build && cd build
cmake .. [OPTIONS]
```
It is possible that during the recovery of the sources of the libraries the branch of the repo of one of the libraries is not good in these cases it is necessary to add the option `-DGIT_BRANCH=next`.
As mentioned above, it is also possible to get the sources of a library locally with the option `-DCPM_<package>_SOURCE=/path/to/your/library`.

```bash
make -j
```

This will take some time.

## 5. Run
```bash
cd ../
./build/vp --gs_luafile conf.lua
```
You should see the following output:
```bash

        SystemC 2.3.4_pub_rev_20200101-GreenSocs --- Feb  1 2022 14:35:03
        Copyright (c) 1996-2019 by all Contributors,
        ALL RIGHTS RESERVED
@0 s /0 (lua): Parse command line for --gs_luafile option (3 arguments)
@0 s /0 (lua): Option --gs_luafile with value conf.lua
Lua file command line parser: parse option --gs_luafile conf.lua
@0 s /0 (lua): Read lua file 'conf.lua'

Info: non_module: Initializing QEMU instance with args:

[...]
Welcome to Buildroot
buildroot login: root
login[176]: root login on 'console'
# ls /
bin           lib           mnt           run           usr
bins          lib64         opt           sbin          var
dev           lib64_debug   proc          sys
etc           linuxrc       qemu.sh       system_lib64
init          media         root          tmp
```

Once the kernel has booted, you can log in with the 'root' account (no password required).

If the virtual platform crashes and doesn't work, it may be due to the LFS files that make up the repo.
You might want to run the 'git lfs pull' command to get all the LFS files.

### 6. Explore Sources

The sc_main(), where the virtual platform is created is in `src/main.cc`.

You can find all the recovered sources in the folder `build/_deps/<package>-src/`.

### 7. Run the test

You can run a test once you have compiled and built the project, just go to your build directory and run the `make test` command.
To run this test you need to be in sudo.

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

## The GreenSocs SystemC simple components library.

This includes simple models such as routers, memories and exclusive monitor. The components are "Loosely timed" only. They support DMI where appropriate, and make use of CCI for configuration.

It also has several unit tests for memory, router and exclusive monitor.

## LIBGSSYNC

The GreenSocs Synchronization library provides a number of different policies for synchronizing between an external simulator (typically QEMU) and SystemC.

These are based on a proposed standard means to handle the SystemC simulator. This library provides a backwards compatibility layer, but the patched version of SystemC will perform better.
## LIBGSUTILS

The GreenSocs basic utilities library contains utility functions for CCI, simple logging and test functions.
It also includes some basic tlm port types
## LIBQEMU-CXX 

Libqemu-cxx encapsulates QEMU as a C++ object, such that it can be instanced (for instance) within a SystemC simulation framework.

The GreenSocs extra components library
--------------------------------------

This library support PCI devices - specifically NVME

## LIBQBOX 

Libqbox encapsulates QEMU in SystemC such that it can be instanced as a SystemC TLM-2.0 model.
## The GreenSocs SystemC Generic UART components.

This includes simple Generic UART models. The components are "Loosely timed" only.

[//]: # (SECTION 10)


[//]: # (SECTION 10 AUTOADDED)

## Information about building and using the base-components library
The base-components library depends on the libraries : Libgsutls, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

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
## Information about building and using the libqemu-cxx library

The libgsutils library does not depend on any library.
## Information about building and using the extra-components library
The extra-components library depends on the libraries : base-components, libgssync, libqemu-cxx, libqbox, libgsutils, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.
## Information about building and using the greensocs Qbox library
The greensocs Qbox library depends on the libraries : base-components, libgssync, libqemu-cxx, libgsutils, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

[//]: # (SECTION 50)


[//]: # (SECTION 50 AUTOADDED)

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
The GreenSocs extra-component library 
-------------------------------------

The extra-component library contains complex components. 

Components available:

 * GPEX : The extra-component Gpex allow you to add devices with the method `add_device(Device &dev)`, you can add for example an NVME disk:
```c++
    m_gpex.add_device(m_disk1);
```
 * NVME : This allows an NVME disk to be added. Initialise `QemuNvmeDisk` with a qemu instance from the libqbox library (see the libqbox section to initiate a qemu instance):

```c++
    QemuNvmeDisk m_disk1("disk1", m_qemu_inst)
```
The class `QemuNvmeDisk` has 3 parameters:
The constructor : `QemuNvmeDisk(const sc_core::sc_module_name &name, QemuInstance &inst)`
- "name" : name of the disk
- "inst" : instance Qemu

The parameters :
- "serial" : Serial name of the nvme disk
- "bloc-file" : Blob file to load as data storage
- "drive-id"

## How to add QEMU OpenCores Eth MAC
### SystemC Code
Here there is an example of SystemC Code:
Create an address map cci parameter
```
cci::cci_param<cci::uint64> m_addr_map_eth;
```

Declare an instance of the qemu ethernet itself and a 'global initiator port' that can be used by any QEMU 'SysBus' initiator.
```
QemuOpencoresEth m_eth;
GlobalPeripheralInitiator m_global_peripheral_initiator;
```

Bind them to the router
```
m_router.add_initiator(m_global_peripheral_initiator.m_initiator);
m_router.add_target(m_eth.regs_socket, m_addr_map_eth, 0x54);
m_router.add_target(m_eth.desc_socket, m_addr_map_eth + 0x400, 0x400);
```

And bind the qemu ethernet at the Generic Interrup Controller
```
m_eth.irq_out.bind(m_gic.spi_in[2]);
```

Then in a constructor initialize them
```
m_addr_map_eth("addr_map_eth", 0xc7000000, "Ethernet MAC base address")
m_eth("ethoc", m_qemu_inst)
m_global_peripheral_initiator("glob-per-init", m_qemu_inst, m_eth)
```

### Linux Kernel
Add driver
```
CONFIG_ETHOC=y
Location:
  │     -> Device Drivers
  │       -> Network device support (NETDEVICES [=y])
  │         -> Ethernet driver support (ETHERNET [=y])
  │           -> OpenCores 10/100 Mbps Ethernet MAC support
```
Add device tree entry
```
ethoc: ethernet@c7000000 {
    compatible = "opencores,ethoc";
    reg = <0x00 0xc7000000 0x2000>;
    interrupts = <0x00 0x02 0x04>;
};
```
Instanciate Qemu
----------------

A QemuManager is required in order to instantiate a Qemu instance. A QemuManager will hold, and maintain the instance until the end of execution. The QemuInstance can contain one or many CPU's and other devices.
To create a new instance you can do this:
```c++
    QemuInstanceManager m_inst_mgr;
```

then you can initialize it by providing the QemuInstance object with the QemuInstanceManager object which will call the `new_instance` method to create a new instance.
```c++
    QemuInstance m_qemu_inst(m_inst_mgr.new_instance(QemuInstance::Target::AARCH64))
```

In order to add a CPU device to an instance they can be constructed as follows:
```c++
    sc_core::sc_vector<QemuCpuArmCortexA53> m_cpus

    m_cpus("cpu", 32, [this] (const char *n, size_t i) { return new QemuCpuArmCortexA53(n, m_qemu_inst); })
```
You can change the CPUs to those listed below in the "CPU" section

Interrupt Controllers and others devices also need a QEMU instance and can be set up as follows:
```c++
    QemuArmGicv3 m_gic("gic", m_qemu_inst);
    QemuUartPl011 m_uart("uart", m_qemu_inst)
```

QEMU Arguments
--------------

The QEMU instance provides the following default arguments :
```
    "-M", "none", /* no machine */
    "-m", "2048", /* used by QEMU to set some interal buffer sizes */
    "-monitor", "null", /* no monitor */
    "-serial", "null", /* no serial backend */
    "-display", "none", /* no GUI */
```

QEMU arguments can be added to an entire instance using the configuration
mechanism. The instance has one parameter `args` that can be used to append a
whitespace separated list of arguments. To enable some qemu traces, one can
set `"qemu-instance-name.args" = "-D file.log -trace pattern1 -trace pattern2"`.

To append a specific QEMU option value you can also use the form
`"qemu-instance-name.args.-OPTION" = "value"`.
The latter cannot be used to append several time the same option, as a parameter definition will override any previous one.

Example :
Using the lua file configuration mechanism to set `-monitor` to enable telnet communication with QEMU, with the QEMU instance "platform.QemuInstance" the lua file should contain :

```lua
["platform.QemuInstance.args.-monitor"] = "tcp:127.0.0.1:55555,server,nowait",
```
To check that the QEMU argument has been added QEMU will report :
`Added QEMU argument: "name of the argument" "value of the argument"`

In the example it's :
`Added QEMU argument : -monitor tcp:127.0.0.1:55555,server,nowait`

Telnet can be used to connector to the monitor as follows:
```bash
$ telnet 127.0.0.1 55555
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
QEMU 5.1.0 monitor - type 'help' for more information
(qemu) quit
quit
Connection closed by foreign host.
```

NOTE :

This should not be used to enable GDB.

Enabling GDB per CPU
--------------------

In order to connect a GDB the CCI parameter `name.of.cpu.gdb_port` must be set a none zero value.

For instance 
```bash
$ ./build/vp --gs_luafile conf.lua -p platform.cpu_1.gdb_port=1234
```
Will open a gdb server on port 1234, for `cpu_1`, and the virtual platform will wait for GDB to connect.


## The components of libqbox
### CPU
The libqbox library supports several CPU architectures such as ARM and RISCV.
- In ARM architectures the library supports the cortex-a53 and the Neoverse-N1 which is based on the cortex-a76 architecture which itself derives from the cortex-a75/73/72.
- In RISCV architecture, the library manages only the riscv64.

### IRQ-CTRL
The library also manages interrupts by providing :
- ARM GICv2
- ARM GICv3 
which are Arm Generic Interrupt Controller.

Then :
- SiFive CLINT
- SiFive PLIC 
which are also Interrupt controller but for SiFive.

### UART
Finally, it has 2 uarts: 
- pl011 for ARM 
- 16550 for more general use

### PORTS
The library also provides socket initiators and targets for Qemu

## QEMU/SystemC parallelism
### QEMU TCG threading mode

QEMU/SystemC integration support 3 threading mechanisms. They determine the threading mode used within the qemu tcg.
This is selected using the QEMU instance parameter `"tcg_mode"` which can take the following three values:

- `coroutine`
      - qemu uses the single thread tcg mode
      - qemu tcg code does not run in parallel of systemC engine
      - useful for determinism (with icount enabled, see below)

- `singlethread`
      - qemu use single thread tcg mode
      - qemu tcg code run in a separate thread of systemc engine


- `multithread` (this is the default)
      - qemu use mutiples threads tcg mode
      - qemu tcg threads run in parallel of systemc engine
      - not every qemu architecture support multithread

To select a threading mode, set the param `"tcg_mode"` on the QEMU instance to one of `"COROUTINE"`, `"SINGLE"` or `"MULTI"`
(The defualt is `"MULTI"`)

QEMU also supports `icount` mode, where timing is based on the number of instructions executed.
Two parameters control icount mode
- `"icount"` enables or disables icount mode
- `"icount_mips_shift"` is the MIPS shift value for icount mode (1 insn = 2^(mips) ns). If this is set to anything other than 0, icount mode is also enabled.

If icount is not selected, QEMU will use 'wall clock' time internally. This is (clearly) non deterministic.

The default is that icount mode is disabled.

_NB icount mode should not be enabled with multi-threading as this is not possible within QEMU. Doing so will cause an error._

### TLM2 Quantum keeper synchronization mode

The GreenSocs synchronization library supports a number of synchronization policies:
- `tlm2`
 - This is standard TLM2 mode, there is no attempt in the quantum keeper to handle multiple threads. This mode should only be used with `COROUTINES` (This will be assumed and does not need to be set).
- `multithread`
 - This is a basic multi-threaded quantum keeper, it will attempt, by default, to keep everything within 2 quantums (+- a quantum).
- `multithread-quantum`
 - This mode attempts to replicate a closer to tlm behaviour, in that things should not advance until everybody has reached the quantum boundry.
- `multithread-unconstrained`
 - This mode allows QEMU to run at it's own pace. This is the _DEFAULT_

_NB, none of the `multithread` based syncronisation policies can be used with COROUTINES, and this will generate an error_

_For deterministic execution enable BOTH `tlm2` synchronisation _and_ `icount` mode._

[//]: # (SECTION 100)


[//]: # (PROCESSED BY doc_merge.pl)
