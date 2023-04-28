

[//]: # (SECTION 0)
# GreenSocs Neoverse N1 virtual platform

## 1. Overview

This release contains an example of a virtual platform based on an ARM Neoverse-N1.
## 2. Requirements

You can build this release natively on Ubuntu 18.04.

Install dependencies:
```bash
apt update && apt upgrade -y
apt install -y make cmake g++ wget flex bison unzip python pkg-config libpixman-1-dev libglib2.0-dev
```

## 3. Fetch the qbox sources

If you have an SSH key:
```bash
cd $HOME
git clone git@gitlab.qualcomm.com:qqvp/platforms/quic-neoverse-n1.git
```

otherwise:
```bash
cd $HOME
git clone https://gitlab.qualcomm.com/qqvp/platforms/quic-neoverse-n1.git
```

this will extract the platform in $HOME/quic-neoverse-n1

## 4. Build the platform

```bash
cd $HOME/quic-neoverse-n1
mkdir build && cd build
cmake .. [OPTIONS]
```
It is possible that during the recovery of the sources of the libraries the branch of the repo of one of the libraries is not good in these cases it is necessary to add the option `-DGIT_BRANCH=next`.
As mentioned above, it is also possible to get the sources of a library locally with the option `-DCPM_<package>_SOURCE=/path/to/your/library`.

```bash
make -j
```

This will take some time.

## 5. Run
### Configure a tun device called qbox0
If you don't have a tun called qbox0 you will need one, for this you need to run the following command:
```bash
sudo ip tuntap add qbox0 mode tap
```
also add ip to the tun interface with:
```bash
sudo ip addr add 10.0.0.1/24 dev qbox0
sudo ip link set dev qbox0 up
```
To test this you can do `ping 10.0.0.1` from vp to host or `ping 10.0.0.2` from host to vp.

### Run the Virtual Platform
```bash
cd ../
./build/vp --gs_luafile conf.lua
```
You should see the following output:
```bash

        SystemC 2.3.4_pub_rev_20200101-GreenSocs --- Aug 23 2021 11:07:13
        Copyright (c) 1996-2019 by all Contributors,
        ALL RIGHTS RESERVED
@0 s /0 (lua): Parse command line for --gs_luafile option (3 arguments)
@0 s /0 (lua): Option --gs_luafile with value conf.lua
Lua file command line parser: parse option --gs_luafile conf.lua
@0 s /0 (lua): Read lua file 'conf.lua'
Booting Linux on physical CPU 0x0000000000 [0x413fd0c1]
Linux version 5.4.58 (salama@DESKTOP-O36AFBD) (gcc version 9.3.0 (Buildroot 2021.02.3)) #1 SMP Mon Jul 12 16:29:53 CEST 2021
Machine model: linux,dummy-virt
earlycon: pl11 at MMIO 0x00000000c0000000 (options '')
printk: bootconsole [pl11] enabled
[...]
Welcome to Buildroot - Log in with username (root) and password (greensocs)
buildroot login: root
Password:
# ls /
bin         lib         lost+found  opt         run         tmp
dev         lib64       media       proc        sbin        usr
etc         linuxrc     mnt         root        sys         var
```

Once the kernel has booted, you can log in with the 'root' account (no password required).

### 6. Explore Sources

The sc_main(), where the virtual platform is created is in `src/main.cc`.

You can find all the recovered sources in the folder `build/_deps/<package>-src/`.

### 7. Run the test

You can run a test once you have compiled and built the project, just go to your build directory and run the `make test` command.
To run this test you need to be in sudo.