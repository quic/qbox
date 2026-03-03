# Networking -- SystemC Generic MAC

The SystemC generic MAC library provides loosely-timed Ethernet
MAC models.

## SystemC Setup

The MAC requires at least 0x8000 of memory space. Initialize
the backend:

```cpp
m_dwmac.set_backend(new NetworkBackendTap("dwmac-backend", "qbox0"));
```

## Linux Kernel Configuration

Enable the DWMAC generic driver:

```
CONFIG_DWMAC_GENERIC=y
Location:
  -> Device Drivers
    -> Network device support (NETDEVICES [=y])
      -> Ethernet driver support (ETHERNET [=y])
        -> STMicroelectronics devices (NET_VENDOR_STMICRO [=y])
          -> STMicroelectronics Multi-Gigabit Ethernet driver (STMMAC_ETH [=y])
            -> STMMAC Platform bus support (STMMAC_PLATFORM [=y])
```

## Device Tree Entry

```dts
gmac0: ethernet@200000 {
    compatible = "snps,dwmac";
    reg = <register space>;
    interrupt-parent = <&interrupt_parent>;
    interrupts = <interrupt_nr>;
    interrupt-names = "macirq";
    phy-mode = "gmii";
};
```

## Buildroot Configuration

Enable SSH:

```
BR2_PACKAGE_OPENSSH=y
BR2_PACKAGE_OPENSSH_SERVER=y
BR2_PACKAGE_OPENSSH_CLIENT=y
```

Enable root password (required for SSH):

```
BR2_TARGET_GENERIC_ROOT_PASSWD=greensocs
```

To permit root login via SSH, add to `/etc/ssh/sshd_config`:

```
PermitRootLogin yes
```

Configure the network interface. Add to
`/etc/network/interfaces`:

```
auto eth0
iface eth0 inet static
    address 192.168.0.97
    netmask 255.255.255.0
```

## Host-Side Setup

Create and configure the TUN interface:

```bash
sudo ip tuntap add dev qbox0 mode tap user $(whoami)
sudo ip addr add 192.168.0.254/24 dev qbox0
sudo ip link set dev qbox0 up
```

After starting the virtual platform, connect via SSH:

```bash
ssh root@192.168.0.97
```

## virtio-net (PCI)

The `virtio_net_pci` class provides a virtio network device
that attaches to a GPEX host bridge as a PCI device. It
requires an existing GPEX host bridge in the platform
configuration.

### CCI Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `mac` | string | `""` | MAC address of the NIC |
| `netdev_str` | string | `type=user` | QEMU netdev string (do not specify ID) |

### Lua Configuration Example

The PCI variant needs a GPEX host bridge. The device is added
to the bridge automatically via its constructor arguments.

```lua
gpex_0 = {
    moduletype = "qemu_gpex",
    args = {"&platform.qemu_inst"},
    bus_master = {bind = "&router.target_socket"},
    pio_iface = {address = 0x3000000, size = 0x10000, bind = "&router.initiator_socket"},
    mmio_iface = {address = 0x40000000, size = 0x40000000, bind = "&router.initiator_socket"},
    ecam_iface = {address = 0x30000000, size = 0x10000000, bind = "&router.initiator_socket"},
    mmio_iface_high = {address = 0x400000000, size = 0x400000000, bind = "&router.initiator_socket"},
    irq_out_0 = {bind = "&gic_0.spi_in_32"},
    irq_out_1 = {bind = "&gic_0.spi_in_33"},
    irq_out_2 = {bind = "&gic_0.spi_in_34"},
    irq_out_3 = {bind = "&gic_0.spi_in_35"},
};

virtio_net_0 = {
    moduletype = "virtio_net_pci",
    args = {"&platform.qemu_inst", "&platform.gpex_0"},
    mac = "52:54:00:12:34:56",
    netdev_str = "type=user,hostfwd=tcp::2222-:22",
};
```

## virtio-net (MMIO)

The `virtio_mmio_net` class provides a virtio network device
using a memory-mapped I/O transport. It is a standalone device
that does not require a PCI host bridge; instead it is mapped
directly into the system address space.

### Sockets

- `socket` -- Target socket for MMIO register access
- `irq_out` -- Interrupt signal output

### CCI Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `netdev_str` | string | `user,hostfwd=tcp::2222-:22` | QEMU netdev string (do not specify ID) |

### Lua Configuration Example

The MMIO variant is configured with a memory-mapped address and
an interrupt output binding. No PCI host bridge is needed.

```lua
virtio_net_0 = {
    moduletype = "virtio_mmio_net",
    args = {"&platform.qemu_inst"},
    mem = {address = 0x1c120000, size = 0x10000, bind = "&router.initiator_socket"},
    irq_out = {bind = "&gic_0.spi_in_18"},
    netdev_str = "type=user,hostfwd=tcp::2222-:22",
};
```
