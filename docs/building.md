# Building QBox

## Windows

### Limitations

The following components are not supported on Windows:

- `component_constructor`
- `legacy_char_backend_stdio`
- `fss`
- `ufs`
- `ufs_lu`
- `vnc`
- `pl031`
- `pflash_cfi`
- `fw_cfg`
- `ramfb`

### Prerequisites

#### Enable Developer Mode

QBox uses symbolic links during compilation, which are disabled by default on Windows. You must enable Developer Mode to allow symbolic link creation without elevated privileges.

To enable Developer Mode, follow the official [Microsoft guide](https://learn.microsoft.com/en-us/windows/apps/get-started/enable-your-device-for-development).

#### Set Up a Dev Drive (Optional)

For improved compilation performance, consider setting up a Dev Drive as described in the [Microsoft documentation](https://learn.microsoft.com/en-us/windows/dev-drive/).

If you configure a Dev Drive, install the MSYS2 environment on this drive to ensure all build operations benefit from the performance improvements. For example, if the Dev Drive is mounted at `D:\`, choose a subdirectory such as `D:\msys64` as the MSYS2 installation path.

### Install MSYS2

Download and install MSYS2 from [https://www.msys2.org](https://www.msys2.org).

After installation, launch the appropriate MSYS2 environment from the Start Menu:

| Host Architecture | Environment |
|-------------------|-------------|
| Windows ARM64 (including Snapdragon) | MSYS2 CLANGARM64 |
| Windows x86_64 | MSYS2 UCRT64 |

> **Note:** The `MSYS2 MINGW64` environment is available but not recommended. See the [MSYS2 environments documentation](https://www.msys2.org/docs/environments/) for details on MSVCRT vs UCRT.

### Install Build Dependencies

Run the following commands in your MSYS2 terminal:

```bash
pacman -Syu
```

After the system update completes (you may need to restart the terminal), install the required packages:

```bash
pacman -S --noconfirm \
    base-devel \
    binutils \
    bison \
    diffutils \
    flex \
    git \
    grep \
    make \
    sed \
    vim \
    tree \
    msys2-runtime-devel \
    ${MINGW_PACKAGE_PREFIX}-toolchain \
    ${MINGW_PACKAGE_PREFIX}-cmake \
    ${MINGW_PACKAGE_PREFIX}-gtk3 \
    ${MINGW_PACKAGE_PREFIX}-libnfs \
    ${MINGW_PACKAGE_PREFIX}-libssh \
    ${MINGW_PACKAGE_PREFIX}-meson \
    ${MINGW_PACKAGE_PREFIX}-ninja \
    ${MINGW_PACKAGE_PREFIX}-pixman \
    ${MINGW_PACKAGE_PREFIX}-pkgconf \
    ${MINGW_PACKAGE_PREFIX}-python \
    ${MINGW_PACKAGE_PREFIX}-SDL2 \
    ${MINGW_PACKAGE_PREFIX}-zstd \
    ${MINGW_PACKAGE_PREFIX}-libelf \
    ${MINGW_PACKAGE_PREFIX}-asio \
    ${MINGW_PACKAGE_PREFIX}-libslirp
```

> **Note:** MSYS2 environments are isolated. If you use multiple environments, packages must be installed separately in each one.

### Configure SSL Certificates (Optional)

If you encounter SSL certificate errors such as:

```
SSL certificate problem: self signed certificate in certificate chain
```

This occurs because MSYS2 does not integrate with the Windows certificate store. To resolve this, copy your organization's certificate files (`.pem` or `.cer`) to the MSYS2 trust store and update the certificate database:

```bash
cp /path/to/certificate.pem /etc/pki/ca-trust/source/anchors/
update-ca-trust
```

For more information, see the [MSYS2 FAQ](https://www.msys2.org/docs/faq/).

### Windows Terminal Integration (Optional)

You can add MSYS2 environments as profiles in Windows Terminal for convenient access.

Add the following entries to your Windows Terminal `settings.json` file. Adjust the paths if MSYS2 is installed in a location other than `C:\msys64`.

#### CLANGARM64 Profile (ARM64)

```json
{
    "commandline": "C:/msys64/msys2_shell.cmd -defterm -here -no-start -clangarm64",
    "guid": "{5c03ab6d-5335-40fd-8b73-c5aab12cf419}",
    "hidden": false,
    "icon": "C:/msys64/clangarm64.ico",
    "name": "CLANGARM64 / MSYS2",
    "startingDirectory": "C:/msys64/home/%USERNAME%"
}
```

#### UCRT64 Profile (x86_64)

```json
{
    "commandline": "C:/msys64/msys2_shell.cmd -defterm -here -no-start -ucrt64",
    "guid": "{17da3cac-b318-431e-8a3e-7fcdefe6d114}",
    "hidden": false,
    "icon": "C:/msys64/ucrt64.ico",
    "name": "UCRT64 / MSYS2",
    "startingDirectory": "C:/msys64/home/%USERNAME%"
}
```
