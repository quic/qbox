# libqbox -- QEMU/SystemC Integration

Libqbox encapsulates QEMU in SystemC such that it can be
instantiated as a SystemC TLM-2.0 model.

## QEMU Instantiation

A `QemuInstanceManager` is required to create a QEMU instance.
The manager holds and maintains instances until the end of
execution. A `QemuInstance` can contain one or many CPUs and
other devices.

### C++ API

```cpp
QemuInstanceManager m_inst_mgr;
QemuInstance m_qemu_inst("qemu_inst", &m_inst_mgr, QemuInstance::Target::AARCH64);
```

Add CPUs:

```cpp
sc_core::sc_vector<cpu_arm_cortexA53> m_cpus;
m_cpus("cpu", 32, [this](const char *n, size_t i) {
    return new cpu_arm_cortexA53(n, m_qemu_inst);
});
```

Add interrupt controllers and devices:

```cpp
arm_gicv3 m_gic("gic", m_qemu_inst);
qemu_pl011 m_uart("uart", m_qemu_inst);
```

### Lua Configuration

```lua
qemu_inst_mgr = {
    moduletype = "QemuInstanceManager"
}
```

Add CPUs via Lua:

```lua
if (ARM_NUM_CPUS > 0) then
    for i = 0, (ARM_NUM_CPUS - 1) do
        local cpu = {
            moduletype = "cpu_arm_cortexA53",
            args = {"&platform.qemu_inst"},
            mem = {bind = "&router.target_socket"},
            has_el2 = true,
            has_el3 = false,
            irq_timer_phys_out = {bind = "&gic_0.ppi_in_cpu_" .. i .. "_" .. ARCH_TIMER_NS_EL1_IRQ},
            irq_timer_virt_out = {bind = "&gic_0.ppi_in_cpu_" .. i .. "_" .. ARCH_TIMER_VIRT_IRQ},
            irq_timer_hyp_out = {bind = "&gic_0.ppi_in_cpu_" .. i .. "_" .. ARCH_TIMER_NS_EL2_IRQ},
            irq_timer_sec_out = {bind = "&gic_0.ppi_in_cpu_" .. i .. "_" .. ARCH_TIMER_S_EL1_IRQ},
            psci_conduit = "hvc",
            mp_affinity = (math.floor(i / 8) << 8) | (i % 8),
            start_powered_off = true,
            cntfrq_hz = 6250000,
        }
        if (i == 0) then
            cpu["rvbar"] = 0x00000000
            cpu["start_powered_off"] = false
        end
        platform["cpu_" .. tostring(i)] = cpu
    end
end
```

Configure an interrupt controller and UART:

```lua
gic_0 = {
    moduletype = "arm_gicv3",
    args = {"&platform.qemu_inst"},
    dist_iface    = {address = 0xc8000000, size = 0x10000, bind = "&router.initiator_socket"},
    redist_iface_0 = {address = 0xc8010000, size = 0x20000, bind = "&router.initiator_socket"},
    num_cpus = ARM_NUM_CPUS,
    redist_region = {ARM_NUM_CPUS / NUM_REDISTS},
    num_spi = 64,
}

qemu_pl011 = {
    moduletype = "qemu_pl011",
    args = {"&platform.qemu_inst"},
    mem = {address = 0xc0000000, size = 0x1000, bind = "&router.initiator_socket"},
    irq_out = {bind = "&gic_0.spi_in_1"},
}
```

## QEMU Arguments

The QEMU instance provides these default arguments:

```
-M none          (no machine)
-m 2048          (internal buffer sizes)
-monitor null    (no monitor)
-serial null     (no serial backend)
```

If no display argument has been set by a component,
`-display none` is also added.

Additional arguments can be appended via the `qemu_args` CCI
parameter on the instance:

```lua
["platform.qemu_inst.qemu_args"] = "-D file.log -trace pattern1 -trace pattern2"
```

To set a specific QEMU option, use the `qemu_args.` prefix:

```lua
["platform.qemu_inst.qemu_args.-monitor"] = "tcp:127.0.0.1:55555,server,nowait"
```

The latter form cannot set the same option multiple times,
since a parameter definition overrides any previous one.

### Telnet Monitor Example

```lua
["platform.qemu_inst.qemu_args.-monitor"] = "tcp:127.0.0.1:55555,server,nowait"
```

Connect with:

```bash
telnet 127.0.0.1 55555
```

Do **not** use QEMU arguments to enable GDB -- use the GDB
port parameter instead.

## GDB Support

To attach GDB to a CPU, set the `gdb_port` CCI parameter to a
non-zero value:

```bash
./build/platforms/platforms-vp --gs_luafile conf.lua -p platform.cpu_1.gdb_port=1234
```

This opens a GDB server on port 1234 for `cpu_1`. The virtual
platform will wait for GDB to connect before proceeding.

## Supported Components

### CPUs

**ARM:**
- Cortex-A53, Cortex-A55, Cortex-A76, Cortex-A710
- Cortex-M7, Cortex-M55
- Cortex-R5, Cortex-R52
- Neoverse-N1, Neoverse-N2

**RISC-V:**
- riscv32, riscv64
- SiFive X280

**Hexagon:**
- Hexagon DSP

### Interrupt Controllers

- **ARM GICv2** -- Generic Interrupt Controller v2
- **ARM GICv3** -- Generic Interrupt Controller v3
  (with optional ITS)
- **ARMv7-M NVIC** -- Nested Vectored Interrupt Controller
- **SiFive PLIC** -- Platform-Level Interrupt Controller
  (RISC-V)
- **RISC-V ACLINT SWI** -- Software interrupt controller
- **Hexagon L2VIC** -- Hexagon L2 vectored interrupt
  controller

### UARTs

- **PL011** -- ARM UART
- **16550** -- General-purpose UART
- **SiFive UART** -- RISC-V UART

### VNC

QBox provides a VNC interface for GPU display outputs.

Single display:

```lua
gpu = {
    moduletype = "virtio_gpu_gl_pci",
    args = ...,
}
platform["gpu_0"] = gpu

vnc = {
    moduletype = "vnc",
    gpu_output = 0,
    args = {"&platform.gpu_0"},
}
platform["vnc_0"] = vnc
```

Multi-display (set `outputs` on the GPU and assign a VNC
session to each):

```lua
gpu = {
    moduletype = "virtio_gpu_gl_pci",
    outputs = 2,
    args = ...,
}
platform["gpu_0"] = gpu

vnc0 = {
    moduletype = "vnc",
    gpu_output = 0,
    args = {"&platform.gpu_0"},
}
vnc1 = {
    moduletype = "vnc",
    gpu_output = 1,
    args = {"&platform.gpu_0"},
}
platform["vnc_0"] = vnc0
platform["vnc_1"] = vnc1
```

The optional `params` field is passed to QEMU for additional
VNC configuration (see QEMU VNC documentation). The fields
`:`, `head=`, and `display=` are generated by the VNC module
and are not forwarded if passed via `params`.

Limitations:
- On macOS, VNC is restricted to `virtio-gpu-pci` GPUs.

### Other Devices

The library also provides TLM socket initiators and targets
for QEMU, as well as additional device models including
timers, PCI devices (virtio-gpu, virtio-net, ivshmem, etc.),
virtio MMIO devices, USB controllers, and more. See the
`qemu-components/` source directory for the full list.

## QEMU/SystemC Parallelism

### TCG Threading Modes

QEMU/SystemC integration supports three threading mechanisms,
selected via the `tcg_mode` parameter on the QEMU instance:

| Value | Mode | Description |
|-------|------|-------------|
| `COROUTINE` | Single-thread, no parallelism | QEMU TCG does not run in parallel with the SystemC engine. Useful for determinism (with icount). |
| `SINGLE` | Single-thread, separate thread | QEMU TCG runs in a separate thread from SystemC. |
| `MULTI` | Multi-thread (default) | QEMU uses multiple TCG threads running in parallel with SystemC. Not all QEMU architectures support this. |

### icount Mode

QEMU supports icount mode where timing is based on
instruction count rather than wall-clock time.

| Parameter | Description |
|-----------|-------------|
| `icount` | Enable/disable icount mode |
| `icount_mips_shift` | MIPS shift value (1 insn = 2^shift ns). Setting this to non-zero also enables icount. |

The default is icount disabled. Without icount, QEMU uses
wall-clock time internally, which is non-deterministic.

icount mode must not be enabled with multi-threading
(`MULTI`), as QEMU does not support this combination.

### TLM2 Quantum Keeper Synchronization Mode

The synchronization library supports several policies (see
[libgssync](libgssync.md) for details):

| Policy | Compatible TCG Modes | Notes |
|--------|---------------------|-------|
| `tlm2` | `COROUTINE` only | Standard TLM2 mode. COROUTINE is assumed automatically. |
| `multithread` | `SINGLE`, `MULTI` | Basic multi-threaded, keeps within ~2 quantums. |
| `multithread-quantum` | `SINGLE`, `MULTI` | Closer to TLM behavior, waits for quantum boundary. This is the **default**. |
| `multithread-unconstrained` | `SINGLE`, `MULTI` | QEMU runs at its own pace. |

None of the `multithread` policies can be used with
`COROUTINE`.

For deterministic execution, enable **both** `tlm2`
synchronization **and** `icount` mode.

## Halt Interface

The halt interface manages the halt state of CPUs. By default,
halt is released (state 0). Halt does not work with
`p_power_off` set to `true`.
