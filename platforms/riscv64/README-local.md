[//]: # (SECTION 0)
# Virtual Platform of RISCV64 for Qualcomm

## 1. Overview

This release contains an example of a virtual platform based on an RISCV64.
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
git@gitlab.qualcomm.com:qqvp/platforms/quic-riscv64.git
```

otherwise:
```bash
cd $HOME
https://gitlab.qualcomm.com/qqvp/platforms/quic-riscv64.git
```

this will extract the platform in $HOME/quic-riscv64

## 4. Build the platform

```bash
cd $HOME/quic-riscv64
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

### Run the Virtual Platform
```bash
cd ../
./build/vp --gs_luafile conf.lua
```
You should see the following output:
```bash

OpenSBI v0.9
   ____                    _____ ____ _____
  / __ \                  / ____|  _ \_   _|
 | |  | |_ __   ___ _ __ | (___ | |_) || |
 | |  | | '_ \ / _ \ '_ \ \___ \|  _ < | |
 | |__| | |_) |  __/ | | |____) | |_) || |_
  \____/| .__/ \___|_| |_|_____/|____/_____|
        | |
        |_|

Platform Name             : freechips,rocketchip-unknown
Platform Features         : timer,mfdeleg
Platform HART Count       : 2
Firmware Base             : 0x70000000
Firmware Size             : 152 KB
Runtime SBI Version       : 0.2
[...]
Welcome to Buildroot
buildroot login: root
# ls /
bin      init     linuxrc  opt      run      tmp
dev      lib      media    proc     sbin     usr
etc      lib64    mnt      root     sys      var

```

Once the kernel has booted, you can log in with the 'root' account (no password required).

For configuration parameters, see conf.lua (and see below)

NB: Some parameters allow to load raw binary files into the memories of the VP
(the `platform.*_blob_file` parameters).  Please note that once at least one of
those parameters is in use, the simulated bootloader of the VP is disabled and
the related bootloader options (`opensbi_file`, `kernel_file`, `dtb_file`) have
no effect.

### 6. Explore Sources

The sc_main(), where the virtual platform is created is in `src/main.cc`.

You can find all the recovered sources in the folder `build/_deps/<package>-src/`.
