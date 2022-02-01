[//]: # (SECTION 0)
# Virtual Platform of ARM Cortex A53 and Hexagon for Qualcomm

## 1. Overview

This release contains an example of a virtual platform based on an ARM Cortex A53 and Hexagon DSP.
## 2. Requirements

You can build this release natively on Ubuntu 18.04.

Install dependencies:
```bash
apt update && apt upgrade -y
apt install -y make cmake g++ wget flex bison unzip python pkg-config libpixman-1-dev libglib2.0-dev libelf-dev
```

## 3. Fetch the qbox sources

If you have an SSH key:
```bash
cd $HOME
git@git.greensocs.com:customers/qualcomm/qualcomm_vp.git
```

otherwise:
```bash
cd $HOME
https://git.greensocs.com/customers/qualcomm/qualcomm_vp.git
```

this will extract the platform in $HOME/qualcomm_vp

## 4. Build the platform

```bash
cd $HOME/qualcomm_vp
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
```bash
cd ../
./build/vp --gs_luafile conf.lua
```
You should see the following output:
```bash

        SystemC 2.3.4_pub_rev_20200101-GreenSocs --- Feb  1 2022 14:35:03
        Copyright (c) 1996-2019 by all Contributors,
        ALL RIGHTS RESERVED
@0 s /0 (lua): Parse command line for --gs_luafile option (3 arguments)
@0 s /0 (lua): Option --gs_luafile with value conf.lua
Lua file command line parser: parse option --gs_luafile conf.lua
@0 s /0 (lua): Read lua file 'conf.lua'

Info: non_module: Initializing QEMU instance with args:

[...]
Welcome to Buildroot
buildroot login: root
login[176]: root login on 'console'
# ls /
bin           lib           mnt           run           usr
bins          lib64         opt           sbin          var
dev           lib64_debug   proc          sys
etc           linuxrc       qemu.sh       system_lib64
init          media         root          tmp
```

Once the kernel has booted, you can log in with the 'root' account (no password required).

If the virtual platform crashes and doesn't work, it may be due to the LFS files that make up the repo.
You might want to run the 'git lfs pull' command to get all the LFS files.

### 6. Explore Sources

The sc_main(), where the virtual platform is created is in `src/main.cc`.

You can find all the recovered sources in the folder `build/_deps/<package>-src/`.

### 7. Run the test

You can run a test once you have compiled and built the project, just go to your build directory and run the `make test` command.
To run this test you need to be in sudo.
