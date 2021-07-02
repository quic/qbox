
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
