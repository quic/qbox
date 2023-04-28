
[//]: # (SECTION 0)

## The GreenSocs SystemC Generic MAC library.

This includes simple ethernet MAC's. The components are "Loosely timed" only. 

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