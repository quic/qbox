# Ubuntu Linux Platform

QBox includes an Ubuntu platform that boots a full Linux
distribution on either AArch64 or RISC-V 64.

## Building the Root Filesystem Image

The `platforms/ubuntu/fw/build_linux_dist_image.sh` script
builds a customized Ubuntu OS rootfs image.

Prerequisites:
- Ubuntu host machine (tested on Ubuntu 22.04.5 LTS)
- Supported distributions: Ubuntu (jammy, noble) and
  Fedora (39, 40) for aarch64; Ubuntu (jammy, noble) for
  riscv64

### Generated Artifacts

| File | Description |
|------|-------------|
| `Image.bin` | Uncompressed Linux kernel image (AArch64 or RISC-V 64) |
| `image_ext4_vmlinuz.bin` | gzip-compressed AArch64 Linux kernel image |
| `image_ext4.img` | ext4 root filesystem image |
| `image_ext4_initrd.img` | initramfs for bringing up the root filesystem |
| `ubuntu.dts` | Device tree source file |
| `ubuntu.dtb` | Device tree blob passed to the kernel by the bootloader |

Use `-h` to list all available script options.

## Running the Platform

### Step-by-Step

Generate the image:

```bash
cd platforms/ubuntu/fw/

# For aarch64:
./build_linux_dist_image.sh -s 4G -p xorg,pciutils -a aarch64

# For riscv64:
./build_linux_dist_image.sh -s 4G -p xorg,pciutils -a riscv64
```

The `Artifacts` directory will be created with all
generated files.

Build QBox from the repository root:

```bash
# For aarch64:
cmake -B build -DLIBQEMU_TARGETS=aarch64
cmake --build build --parallel

# For riscv64:
cmake -B build -DLIBQEMU_TARGETS=riscv64
cmake --build build --parallel
```

Run the virtual platform from the build directory:

```bash
# For aarch64:
./build/platforms/platforms-vp -l ../platforms/ubuntu/conf_aarch64.lua

# For riscv64:
./build/platforms/platforms-vp -l ../platforms/ubuntu/conf_riscv64.lua
```

### Using the `ubuntu` CMake Target

To build the image and run the platform in one step:

```bash
# For aarch64:
cmake -B build -DUBUNTU_ARCH=aarch64 -DLIBQEMU_TARGETS=aarch64
cmake --build build --parallel

# For riscv64:
cmake -B build -DUBUNTU_ARCH=riscv64 -DLIBQEMU_TARGETS=riscv64
cmake --build build --parallel

# Build image and run:
make -C build ubuntu
```

## Login Credentials

```
Username: root
Password: root
```
