[//]: # (SECTION 0)

# QBox

 

##  Overview

 

This package contains a number of components:

*  GreenSocs which contains basic systemc components, utilities, and the synchronization mechanism.

* libqbox which is the integration with QEMU

* libqemu-cxx is the wrapper arround QEMU itself, into C++

 

There are also several example platforms in the platforms/ directory. Please choose a platform to experiment with and see the documentation in the platform.

 

##  Requirements

 

You can build this release natively on Ubuntu 20.

 

Install dependencies:

```bash

apt update && apt upgrade -y

apt install -y make cmake g++ wget flex bison unzip python3 python3-dev python3-pip pkg-config libpixman-1-dev libglib2.0-dev ninja-build

pip3 install numpy meson

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

 

 

The library assumes the use of C++14, and is compatible with SystemC versions from SystemC 2.3.1a.

 

## More documentation

 

More documentation, including doxygen generated API documentation can be found in the `/docs` directory.

### Gitlab Pages / GitHub Pages

You can also find a complete documentation with the implementation of Gitlab/GitHub Pages.

Gitlab Pages URL:

`https://qqvp.gitlab-pages.qualcomm.com/Qbox/`

GitHub Pages URL:

`https://quic.github.io/qbox/`

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

## LIBQBOX

 

Libqbox encapsulates QEMU in SystemC such that it can be instanced as a SystemC TLM-2.0 model.

 

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

## Information about building and using the greensocs Qbox library

The greensocs Qbox library depends on the libraries : base-components, libgssync, libqemu-cxx, libgsutils, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

 

[//]: # (SECTION 50)

 

 

[//]: # (SECTION 50 AUTOADDED)

 

## The GreenSocs component library memory

The memory component allows you to add memory when creating an object of type `Memory("name",size)`.

 

The memory component consists of a simple target socket :`tlm_utils::simple_target_socket<Memory> socket`

 

## The GreenSocs component library router

The router offers `add_target(socket, base_address, size)` as an API to add components into the address map for routing. (It is recommended that the addresses and size are CCI parameters).

 

It also allows to bind multiple initiators with `add_initiator(socket)` to send multiple transactions.

So there is no need for the bind() method offered by sockets because the add_initiator method already takes care of that.

## The GreenSocs component library PythonBinder

The python binder component is a systemc model used to initiate or react to systemc TLM transactions from the python programming language. The model only exposes the minimum set of systemc/TLM features to python for mainly implementing python based backends for I/O models (e.g., stdio backend for UART) and models which can react to systemc initiated transactions utilising the python awesome language with a rich set of useful packages. The model uses a C++ library called pybind11: https://pybind11.readthedocs.io/en/stable/ to embed a python interpreter within the virtual platform process and expose a set of systemc C++ features to python scripts using pybind11 embedded modules capability: https://pybind11.readthedocs.io/en/stable/advanced/embedding.html. 

The set of modules exposed from C++ to python are:  
(For more information about systemc/TLM types and functions: https://www.accellera.org/images/downloads/standards/systemc/systemc-2.3.1.tgz):
* sc_core: (e.g., ```from sc_core import sc_time_unit``` )  
    * sc_time_unit: maps to C++ sc_core::sc_time_unit enum (SC_FS, SC_PS, SC_NS, SC_US, SC_MS, SC_SEC).
    * sc_time: maps to C++ sc_core::sc_time:
        * sc_time(): default constructor.
        * sc_time(value:float, unit:sc_core.sc_time_unit): constructor.
        * sc_time(value:float, unit:str): constructor (convert time unit from string: "fs"/"SC_FS"->SC_FS, "ps"/"SC_PS"->SC_PS, "ns"/"SC_NS"->SC_NS, ...)
        * sc_time(value:float, scale:bool): constructor.
        * sc_time(value:int, scale:bool): constructor.
        * from_seconds(value:float) -> sc_core.sc_time
        * from_value(value:int) -> sc_core.sc_time 
        * from_string(unit:str) -> sc_core.sc_time
        * to_default_time_units() -> float
        * to_double() -> float
        * to_seconds() -> float
        * to_string() -> str
        * \__repr\__() -> str
        * value() -> int
        * A set of overloaded operators:
            * sc_core.sc_time + sc_core.sc_time -> sc_core.sc_time
            * sc_core.sc_time - sc_core.sc_time -> sc_core.sc_time
            * sc_core.sc_time / sc_core.sc_time -> float
            * sc_core.sc_time / float -> sc_core.sc_time
            * sc_core.sc_time * float -> sc_core.sc_time
            * float * sc_core.sc_time -> sc_core.sc_time
            * sc_core.sc_time % sc_core.sc_time -> sc_core.sc_time
            * sc_core.sc_time += sc_core.sc_time -> sc_core.sc_time
            * sc_core.sc_time -= sc_core.sc_time -> sc_core.sc_time
            * sc_core.sc_time %= sc_core.sc_time -> sc_core.sc_time
            * sc_core.sc_time *= double() -> sc_core.sc_time
            * sc_core.sc_time /= double() -> sc_core.sc_time
            * sc_core.sc_time == sc_core.sc_time -> bool
            * sc_core.sc_time != sc_core.sc_time -> bool
            * sc_core.sc_time <= sc_core.sc_time -> bool
            * sc_core.sc_time >= sc_core.sc_time -> bool
            * sc_core.sc_time > sc_core.sc_time -> bool
            * sc_core.sc_time < sc_core.sc_time -> bool
    * sc_event: maps to C++ sc_core::sc_event:
        * sc_event(): default constructor.
        * sc_event(name:str): constructor.
        * notify() -> None
        * notify(t:sc_core.sc_time) -> None
        * notify(value:float, unit:sc_core.sc_time_unit) -> None
        * notify_delayed() -> None
        * notify_delayed(sc_core.sc_time) -> None
        * notify_delayed(value:float, unit:sc_time_unit) -> None
    * sc_spawn_options: maps to C++ sc_core::sc_spawn_options:
        * sc_spawn_options(): default constructor.
        * dont_initialize() -> None
        * is_method() -> bool
        * set_stack_size(value:int) -> None
        * set_sensitivity(gs.async_event) -> None
        * set_sensitivity(sc_core.sc_event) -> None
        * spawn_method() -> None
    * wait(t:sc_core.sc_time) -> None
    * wait(ev:sc_core.sc_event) -> None
    * wait(ev:gs.sc_event) -> None
    * sc_spawn(f:Callable, name:str, opts:sc_spawn_options) -> None
* gs:
    * async_event: mpas to C++ gs::async_event:
        * async_event(start_attached:bool): constructor
        * async_notify() -> None
        * notify(delay:sc_core.sc_time) -> None
        * async_attach_suspending() -> None
        * async_detach_suspending() -> None
        * enable_attach_suspending(e:bool) -> None
* tlm_generic_payload:
    * tlm_command: maps to C++ tlm::tlm_command enum (TLM_READ_COMMAND, TLM_WRITE_COMMAND, TLM_IGNORE_COMMAND)
    * tlm_response_status: maps to C++ tlm::tlm_response_status enum (TLM_OK_RESPONSE, TLM_INCOMPLETE_RESPONSE, TLM_GENERIC_ERROR_RESPONSE, TLM_ADDRESS_ERROR_RESPONSE, TLM_COMMAND_ERROR_RESPONSE, TLM_BURST_ERROR_RESPONSE, TLM_BYTE_ENABLE_ERROR_RESPONSE)
    * tlm_generic_payload: maps to C++ tlm::tlm_generic_payload with most of its C++ member functions:
        * get_address() -> int
        * set_address(addr:int) -> None
        * is_read() -> bool
        * is_write() -> bool
        * set_read() -> None
        * set_write() -> None
        * get_command() -> tlm_command
        * set_command(command:tlm_command) -> None
        * is_response_ok() -> bool
        * is_response_error() -> bool
        * get_response_status() -> tlm_response_status
        * set_response_status(status:tlm_response_status) -> None
        * get_response_string() -> str
        * get_streaming_width() -> int
        * set_streaming_width(width:int) -> None
        * set_data_length(len:int) -> None
        * get_data_length() -> int
        * set_data_ptr(numpy.typing.NDArray[uint8]) -> None: if the transaction will be created in python land.
        * set_data(numpy.typing.NDArray[uint8]) -> None: if the transaction is created in C++ land and reused from python.
        * get_data() -> numpy.typing.NDArray[uint8]
        * set_byte_enable_length(len:int) -> None
        * get_byte_enable_length() -> int
        * set_byte_enable_ptr(numpy.typing.NDArray[uint8]) -> None: if the transaction will be created in python land.
        * set_byte_enable(numpy.typing.NDArray[uint8]) -> None: if the transaction is created in C++ land and reused from python.
        * get_byte_enable() -> numpy.typing.NDArray[uint8]
        * \__repr\__() -> str
    * cpp_shared_vars: it has these python global variables shared from C++ land:
        * module_args: a python string which includes the command line arguments passed to the PythonBinder modeule as a CCI parameter named: py_module_args
    * tlm_do_b_transport:
        * do_b_transport(id:int, trans:tlm_generic_payload.tlm_generic_payload, delay:sc_core.sc_time) -> None: initiate a b_transport call from python, the function maps to C++ initiator_sockets[id]->b_transport(trans, delay). 
    * initiator_signal_socket:
        * write(id:int, value:bool) -> None: write value using C++ initiator_signal_sockets[id].

The input to the model is a sc_vector<tlm_utils::simple_target_socket_tagged_b<...>> target_sockets.  
A python b_transport(id:int, trans:tlm_generic_payload.tlm_generic_payload, delay:sc_ccore.sc_time) -> None: can be implemented to react to transactions on target_sockets[id].


TLM transactions can also be initiated from the python script by calling:  
tlm_do_b_transport.do_b_transport(id:int, trans:tlm_generic_payload.tlm_generic_payload, delay:sc_core.sc_time) -> None.  
so the PythonBinder has also an ouptut sc_vector<tlm_utils::simple_target_socket_tagged_b<...>> initiator_sockets, and calls to python tlm_do_b_transport.do_b_transport(id, trans, delay) will be translated to C++ initiator_sockets[id]->b_transport(trans, delay).


The model has an input sc_core::sc_vector<TargetSignalSocket<bool>> target_signal_sockets.
signals can be handled from inside the python script using:  
target_signal_cb(id: int, value: bool) -> None.


Also the python script can initiate signal writes on sc_core::sc_vector<InitiatorSignalSocket<bool>> initiator_signal_sockets output signals using:  
write(id:int, value:bool) -> None  
this will correspond to C++ initiator_signal_sockets[id]->write(value).

This set of systemc simulation callbacks can also be implemented in python for the PythonBinder model:
* before_end_of_elaboration() -> None
* end_of_elaboration() -> None
* start_of_simulation() -> None
* end_of_simulation() -> None

PythonBinder uses this set of CCI parameters:
* py_module_name: name of python script with module implementation (with or without .py suffix).
* py_module_dir: path of the directory which contains <py_module_name>.py.
* py_module_args: a string of command line arguments to be passed to the module.
* tlm_initiator_ports_num: number of tlm initiator ports.
* tlm_target_ports_num: number of tlm target ports.
* initiator_signals_num: number of initiator signals.
* target_signals_num: number of target signals.

For examples of how to use PythonBinder:
* tests/base-components/python-binder (PythonBinder test bench).
* py-models/py-uart.py (an example stdio backend for include/greensocs/systemc-uarts/uart-pl011.h UART model).

Please notice that:
To build Qbox without including PythonBinder and its pybind11 dependencies, use this cmake option -DWITHOUT_PYTHON_BINDER=ON

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

    

## Instanciate Qemu

A QemuManager is required in order to instantiate a Qemu instance. A QemuManager will hold, and maintain the instance until the end of execution. The QemuInstance can contain one or many CPU's and other devices.

To create a new instance you can do this:

```lua

    qemu_inst_mgr = {
        moduletype = "QemuInstanceManager"
    },

```

 

In order to add a CPU device to an instance they can be constructed as follows:

```lua

if (ARM_NUM_CPUS > 0) then
    for i=0,(ARM_NUM_CPUS-1) do
        local cpu = {
            moduletype = "cpu_arm_cortexA53";
            args = {"&platform.qemu_inst"};
            mem = {bind = "&router.target_socket"};
            has_el2 = true;
            has_el3 = false;
            irq_timer_phys_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_NS_EL1_IRQ},
            irq_timer_virt_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_VIRT_IRQ},
            irq_timer_hyp_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_NS_EL2_IRQ},
            irq_timer_sec_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_S_EL1_IRQ},
            psci_conduit = "hvc";
            mp_affinity = (math.floor(i / 8) << 8) | (i % 8);
            start_powered_off = true;
            cntfrq_hz = 6250000,
        };
        if (i==0) then
            cpu["rvbar"] = 0x00000000;
            cpu["start_powered_off"] = false;
        end
        platform["cpu_"..tostring(i)]=cpu;

        platform["gic_0"]["irq_out_" .. i] = {bind="&cpu_"..i..".irq_in"}
        platform["gic_0"]["fiq_out_" .. i] = {bind="&cpu_"..i..".fiq_in"}
        platform["gic_0"]["virq_out_" .. i] = {bind="&cpu_"..i..".virq_in"}
        platform["gic_0"]["vfiq_out_" .. i] = {bind="&cpu_"..i..".vfiq_in"}
    end
end

```

You can change the CPUs to those listed below in the "CPU" section

Interrupt Controllers and others devices also need a QEMU instance and can be set up as follows:

```c++

    gic_0=  {
        moduletype = "arm_gicv3",
        args = {"&platform.qemu_inst"},
        dist_iface    = {address=0xc8000000, size= 0x10000, bind = "&router.initiator_socket"};
        redist_iface_0= {address=0xc8010000, size=0x20000, bind = "&router.initiator_socket"};
        num_cpus = ARM_NUM_CPUS,
        -- 64 seems reasonable, but can be up to
        -- 960 or 987 depending on how the gic
        -- is used MUST be a multiple of 32
        redist_region = {ARM_NUM_CPUS / NUM_REDISTS};
        num_spi=64};

    qemu_pl011 = {
        moduletype = "qemu_pl011",
        args = {"&platform.qemu_inst"};
        mem = {address= 0xc0000000,
                                    size=0x1000, 
                                    bind = "&router.initiator_socket"},
        irq_out = {bind = "&gic_0.spi_in_1"},
    };

```

 

Qemu instances can be configured using the parameter `<instance_name>.qemu_args="<string>"` in which case the string will be passed as command line parameter(s) to the QEMU instance.

 

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

# Ubuntu image

You can build a customized Ubuntu OS rootfs image with a script called `build_linux_dist_image.sh`.
(Tested on Ubuntu 20.04.5 LTS host machine)

## Run it by hand

You can test this script with a virtual platform in `platforms/ubuntu/`.
The script is in `platforms/ubuntu/fw/`.

build_linux_dist_image.sh is used to build the Ubuntu OS artifacts.
The output build artifacts are:
- Image.bin (uncompressed AARch64 Linux kernel image with efi stub)
- image_ext4_vmlinuz.bin (gzip compressed AARch64 Linux kernel image)
- image_ext4.img (ext4 root file system image)
- image_ext4_initrd.img (initramfs to bring up the root file system)
- image.ext4.manifest (list of all packages included in the image.ext4)
- ubuntu.dts (device tree source file to build ubuntu.dtb)
- ubuntu.dtb (device tree blob passed to the kernel Image by boot loader)

So if you want to test this platform, you should go in `platforms/ubuntu/fw/`and generate the ubuntu images
like that:

```
./build_linux_dist_image.sh -s 4G -p xorg,pciutils
```

It will create a `Artifacts` directory with all of the thing needed inside it.
If you want to run and test this ubuntu platform, you should go back at the top level 
of the repository and build the project.

And then run the platform in your build directory:
```
./platforms/platforms-vp -l ../platforms/ubuntu/conf.lua
```

## Run the custom target 'ubuntu'

More simple, you can just run the custom target 'ubuntu' to build and run a customized ubuntu platform.

You should just go in your build directory and run:
```
make ubuntu
```

You will need to use these login info:

```
username: root
password: root
```

[//]: # (SECTION 100)

 

 

[//]: # (PROCESSED BY doc_merge.pl)
