[//]: # (SECTION 0)
# Virtual Platform of ARM Cortex A53 and Hexagon for Qualcomm

## 1. Overview

This release contains an example of a virtual platform based on an ARM Cortex A53 and Hexagon DSP.
## 2. Requirements

You can build this release natively on Ubuntu 18.04.

Install dependencies:
```bash
apt update && apt upgrade -y
apt install -y make build-essential cmake g++ wget flex bison unzip python python3-pip iproute2 ninja-build pkg-config libpixman-1-dev libglib2.0-dev git wget curl libelf-dev libvirglrenderer-dev libepoxy-dev libgtk-3-dev libsdl2-dev
```

_NOTE for Ubuntu 18, you will need a newer version of cmake._
```bash
 curl -L https://github.com/Kitware/CMake/releases/download/v3.20.0-rc4/cmake-3.20.0-rc4-linux-x86_64.tar.gz | tar -zxf -
 ./cmake-3.20.0-rc4-linux-x86_64/bin/cmake
```

## 3. Fetch the qbox sources

If you have an SSH key:
```bash
cd $HOME
git clone git@git.greensocs.com:customers/qualcomm/qualcomm_vp.git
git lfs pull
```

otherwise:
```bash
cd $HOME
git clone https://git.greensocs.com/customers/qualcomm/qualcomm_vp.git
git lfs pull
```

this will extract the platform in $HOME/qualcomm_vp
(See below for more options to handle passwords)

## 4. Build the platform

```bash
cd $HOME/qualcomm_vp
mkdir build && cd build
cmake .. [OPTIONS]

make -j
```

_NB. on ubuntu use the cmake you downloaded previously, e.g._
```bash
../cmake-3.20.0-rc4-linux-x86_64/bin/cmake -B build
cd build
make -j
```

This will take some time.
For other build and make options, please see below.

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


### 6. Run with GPU
1. Get the openGL linux image from [here](https://gitlab.qualcomm.com/qqvp/firmware-images) and put it in fw/fastrpc-images/images
2. Copy the filesystem image:
```
    cp /prj/qct/llvm/target/vp_qemu_llvm/images/gki/filesystem.bin ./bsp/linux/extras/fs/filesystem.bin
```
3. Export the Xvfb display
```
    export DISPLAY=:1
```
4. Run the vp with the conf.lua
5. Login with username root
6. Set DISPLAY
```
    export DISPLAY=:0
```
7. Demo
```
    glxgears
```

#### 6.1 Troubleshooting

The following error indicates that an unspecified error (`VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200`)
was generated while processing a virtio-gpu command (`VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING = 0x106`).

```sh
virtio_gpu_virgl_process_cmd: ctrl 0x106, error 0x1200
```

This could mean that the guest kernel requires a more recent version of QEMU or
virglrenderer with support to such a command or some of its parameters.
In this case, you could either:

- update the virglrenderer library installed in your system and recompile quic-vp
- downgrade the guest kernel to an older version
- disable the GPU device by setting the `platform["with_gpu"]` option to `false`

### 7. Explore Sources

The sc_main(), where the virtual platform is created is in `src/main.cc`.

You can find all the recovered sources in the folder `build/_deps/<package>-src/`.

### 8. Run the test
You can run a test once you have compiled and built the project, just go to your build directory and run the `make test` command.
To run this test you need to be in sudo.