
[//]: # (SECTION 0)
The GreenSocs extra components library
--------------------------------------

This library support PCI devices - specifically NVME

[//]: # (SECTION 10)
## Information about building and using the extra-components library
The extra-components library depends on the libraries : base-components, libgssync, libqemu-cxx, libqbox, libgsutils, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

[//]: # (SECTION 50)
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