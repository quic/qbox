

[//]: # (SECTION 0)
## LIBQBOX 

Libqbox encapsulates QEMU in SystemC such that it can be instanced as a SystemC TLM-2.0 model.

[//]: # (SECTION 10)
## Information about building and using the greensocs Qbox library
The greensocs Qbox library depends on the libraries : base-components, libgssync, libqemu-cxx, libgsutils, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

[//]: # (SECTION 50)
## Instanciate Qemu
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
