

[//]: # (SECTION 0)
# Virtual Platform of ARM Cortex M55

## 1. Overview

This release contains an example of a virtual platform based on an ARM Cortex M55

The CPU model is n a remote process, which is connected to from the main process via a remote procedure call mechanism which uses the SystemC TLM-2.0 (IEEE 1666) API.

## 2. Requirements

You can build this release natively on Ubuntu 18.04.

Install dependencies:
```bash
apt update && apt upgrade -y
apt install -y make cmake g++ wget flex bison unzip python pkg-config libpixman-1-dev libglib2.0-dev
```


## 4. Build the platform

```bash
cmake -B build [OPTIONS]
make -j
```

This will take some time.

## 5. Run
### Run the Virtual Platform
```bash
cd ../
./build/vp --gs_luafile conf.lua
```
You should see the following output:
```bash

        SystemC 2.3.4_pub_rev_20200101-GreenSocs --- Sep 21 2021 14:57:41
        Copyright (c) 1996-2019 by all Contributors,
        ALL RIGHTS RESERVED
@0 s /0 (lua): Parse command line for --gs_luafile option (3 arguments)
@0 s /0 (lua): Option --gs_luafile with value conf.lua
Lua file command line parser: parse option --gs_luafile conf.lua
@0 s /0 (lua): Read lua file 'conf.lua'
```

## 5. Making use of the CPU model in other simulation contexts.

The remove CPU model may be used in any systemc context, a convenience class is provided in main.cc which can simply be placed in a SystemC model. The class handles the executable name for the remote model and passing arguments to it (which can, for intance, be used to specify configuration parameters).

The remote device can be found in remote_cpu.cc, which contains the actual CPU model. Some configuration parameters are pre-set in this model. Confguration parameters can be passed on the command line using the syntax ```-p key=value```, a lua configuration file can be added (with the syntax ```--gs_luafile <filename>```) or CCI parameters can be automatically transferred to the remote model (all parameters under the CPUPassRPC class will be automatically transferred).

See remote_cpu.cc for some of the parameters which may be useful to alter, specifically the number of IRQ's

