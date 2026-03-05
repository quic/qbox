# QBox

SystemC/QEMU co-simulation framework for virtual platform
development.

## Overview

QBox integrates QEMU into SystemC as a TLM-2.0 model, enabling
cycle-approximate simulation of complete hardware platforms. The
project consists of:

- **libqemu-cxx** -- C++ wrapper around QEMU
- **libqbox** -- SystemC TLM-2.0 integration layer for QEMU
- **Base components** -- SystemC models (routers, memories,
  loaders, exclusive monitor)
- **QEMU device models** -- CPUs, interrupt controllers, UARTs,
  timers, PCI devices, and more
- **Example platforms** -- Reference implementations in `platforms/`

### Supported Architectures

- **ARM:** Cortex-A53/A55/A76/A710, Cortex-M7/M55,
  Cortex-R5/R52, Neoverse-N1/N2
- **RISC-V:** riscv32, riscv64, SiFive X280
- **Hexagon:** Hexagon DSP

## Requirements

Install dependencies using the provided script (supports Ubuntu
20.04/22.04/24.04 and macOS 14/15):

```bash
sudo scripts/install_dependencies.sh
```

## Building

```bash
# Basic build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# With specific QEMU targets
cmake -B build -DLIBQEMU_TARGETS="aarch64;riscv64"

# Download all dependencies via CPM
cmake -B build -DCPM_DOWNLOAD_ALL=ON

# Install
cmake --install build
```

### CMake Options

| Option | Description |
|--------|-------------|
| `CMAKE_INSTALL_PREFIX` | Install directory for the package and binaries |
| `CMAKE_BUILD_TYPE` | `DEBUG` or `RELEASE` |
| `LIBQEMU_TARGETS` | Semicolon-separated list of QEMU targets (e.g., `aarch64;riscv64;hexagon`) |
| `CPM_DOWNLOAD_ALL` | Download all dependencies via CPM |
| `CPM_SOURCE_CACHE` | Directory to cache downloaded packages |
| `WITHOUT_PYTHON_BINDER` | Build without the PythonBinder component |
| `GS_ENABLE_LTO` | Enable Link-Time Optimization |
| `UBUNTU_ARCH` | Architecture for Ubuntu platform builds (`aarch64` or `riscv64`) |

### Dependency Management

QBox uses [CPM](https://github.com/cpm-cmake/CPM.cmake) to find
and/or download missing dependencies. CPM uses CMake's standard
`find_package` mechanism to locate installed packages.

**Environment variables:**

- `SYSTEMC_HOME` -- Path to a locally installed SystemC
- `CCI_HOME` -- Path to a locally installed SystemC CCI

**Specifying package locations:**

- `<package>_ROOT` -- Point to a specific installed package
  location
- `CPM_<package>_SOURCE_DIR` -- Use your own source directory
  for a package
- `CMAKE_MODULE_PATH` -- Additional search paths for CMake
  modules

Example using a custom SystemC CCI source:

```bash
cmake -B build -DCPM_SystemCCCI_SOURCE=/path/to/your/cci/source
```

**Offline / cached builds:**

```bash
cmake -B build -DCPM_SOURCE_CACHE=$(pwd)/Packages
```

This populates the `Packages` directory. If a directory named
`Packages` exists at the project root, the build system will
automatically use it as a source cache.

### Testing

```bash
# Run all tests
ctest --test-dir build

# Run tests with verbose output
ctest --test-dir build --output-on-failure

# Run a specific test
ctest --test-dir build -R <test_name>

# Or via make
make -C build test
```

### Troubleshooting

**CMake cache issues:** CMake caches compiled modules in
`~/.cmake/`. If you are picking up the wrong version of a
module, it is safe to delete this cache directory.

**Corrupted build directory:** Remove and reconfigure:

```bash
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

**Missing QEMU targets:** Rebuild with the target included:

```bash
cmake -B build -DLIBQEMU_TARGETS="aarch64;riscv64"
```

## Quick Start: Ubuntu Platform (AArch64)

```bash
# Build the rootfs image
cd platforms/ubuntu/fw/
./build_linux_dist_image.sh -s 4G -p xorg,pciutils -a aarch64
cd ../../..

# Build QBox with AArch64 target
cmake -B build -DLIBQEMU_TARGETS=aarch64
cmake --build build --parallel

# Run the virtual platform
./build/platforms/platforms-vp -l platforms/ubuntu/conf_aarch64.lua
```

## Documentation

- **[Configuration](docs/configuration.md)** -- CCI parameters,
  Lua configuration, YAML, ConfigurableBroker

### Component Libraries

- **[libqbox](docs/libqbox.md)** -- QEMU/SystemC integration,
  CPUs, interrupt controllers, UARTs, VNC, parallelism
- **[libqemu-cxx](docs/libqemu-cxx.md)** -- Low-level QEMU C++
  wrapper
- **[libgssync](docs/libgssync.md)** -- Synchronization policies
  and suspend/unsuspend interface
- **[libgsutils](docs/libgsutils.md)** -- CCI utilities and TLM
  port types
- **[Base components](docs/base-components.md)** -- Router,
  memory, loader, memory dumper
- **[Extra components](docs/extra-components.md)** -- GPEX, NVME,
  OpenCores Ethernet
- **[PythonBinder](docs/python-binder.md)** -- Python integration
  for SystemC models
- **[Networking](docs/networking.md)** -- SystemC Ethernet MAC
  setup and host configuration
- **[Character backends](docs/backends.md)** -- stdio, socket,
  and file backends for UART I/O
- **[Monitor](docs/monitor.md)** -- Web-based simulation
  monitoring interface

### Platforms

- **[Ubuntu](docs/platforms/ubuntu.md)** -- Ubuntu Linux platform
  (AArch64 and RISC-V 64)

### Tutorials and Examples

- **[hello-qbox](examples/hello-qbox/)** -- Step-by-step
  tutorial that walks you through building a minimal AArch64
  virtual platform (Cortex-A53, RAM, UART) from scratch

## C++ Standard

QBox requires C++14 and is compatible with SystemC versions from
SystemC 2.3.1a.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

See [LICENSE](LICENSE).
