

[//]: # (SECTION 0)
## LIBQBOX 

Libqbox encapsulates QEMU in SystemC such that it can be instanced as a SystemC TLM-2.0 model.

[//]: # (SECTION 10)
## Information about building and using the greensocs Qbox library
The greensocs Qbox library depends on the libraries : base-components, libgssync, libqemu-cxx, libgsutils, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

[//]: # (SECTION 50)
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
## Halt Interface

The libqbox library allows to manage the halt state of the different CPUs that are used. The halt will allow a cpu to be in "standby".

Indeed, by default the halt state is released (state 0). It is important to note that the halt does not work with the power_off set (parameter p_power_off set to true).