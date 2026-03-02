# Extra Components

The extra-components library provides complex components
including PCI devices and peripherals.

## GPEX (Generic PCI Express)

Add PCI devices to a GPEX host bridge using the `add_device`
method:

```cpp
m_gpex.add_device(m_disk1);
```

## NVME Disk

The `nvme` class provides an NVME disk that attaches to a GPEX
host bridge.

Constructor:
`nvme(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex)`

### CCI Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `serial` | component name | Serial name of the NVME disk |
| `blob_file` | (required) | File to use as data storage |
| `max_ioqpairs` | 64 | Maximum I/O queue pairs (passed to QEMU) |

The `blob_file` parameter is required -- the device will fail
to initialize without it.

## OpenCores Ethernet MAC

The `opencores_eth` class wraps the QEMU OpenCores Ethernet MAC
device.

Constructor:
`opencores_eth(const sc_core::sc_module_name& n, QemuInstance& inst)`

### Sockets

- `regs_socket` -- Register access (target socket)
- `desc_socket` -- Descriptor access (target socket)
- `irq_out` -- Interrupt output

### CCI Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `mac` | `00:11:22:33:44:55` | MAC address |
| `netdev_str` | `user,hostfwd=tcp::2222-:22` | QEMU netdev string (do not specify ID) |

### Linux Kernel Configuration

Enable the driver:

```
CONFIG_ETHOC=y
Location:
  -> Device Drivers
    -> Network device support (NETDEVICES [=y])
      -> Ethernet driver support (ETHERNET [=y])
        -> OpenCores 10/100 Mbps Ethernet MAC support
```

### Device Tree Entry

```dts
ethoc: ethernet@c7000000 {
    compatible = "opencores,ethoc";
    reg = <0x00 0xc7000000 0x2000>;
    interrupts = <0x00 0x02 0x04>;
};
```
