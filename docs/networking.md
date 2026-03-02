# Networking -- SystemC Generic MAC

The SystemC generic MAC library provides loosely-timed Ethernet MAC models.

## SystemC Setup

The MAC requires at least 0x8000 of memory space. Initialize the backend:

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

Configure the network interface. Add to `/etc/network/interfaces`:

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
