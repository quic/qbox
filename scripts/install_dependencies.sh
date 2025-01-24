#!/usr/bin/env sh

# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause

unsupported_operating_system() {
    echo "Error: Unsupported operating system"
    exit 1
}

if { [ -e /etc/os-release ] && OS_RELEASE="/etc/os-release"; } || \
   { [ -e /usr/lib/os-release ] && OS_RELEASE="/usr/lib/os-release"; }; then
    # shellcheck source=/dev/null
    . "${OS_RELEASE}"

    if [ "${ID}" = "ubuntu" ]; then
        apt update

        if [ "${VERSION_ID}" = "24.04" ]; then
            apt install cmake g++ gcc git libasio-dev libelf-dev libepoxy-dev \
                libglib2.0-dev libpixman-1-dev libsdl2-dev \
                libvirglrenderer-dev meson ninja-build ocl-icd-opencl-dev \
                python3 python3-dev python3-numpy python3-venv
        elif [ "${VERSION_ID}" = "22.04" ]; then
            apt install cmake g++ gcc git libasio-dev libelf-dev libepoxy-dev \
                libglib2.0-dev libpixman-1-dev libsdl2-dev \
                libvirglrenderer-dev meson ninja-build ocl-icd-opencl-dev \
                python3 python3-dev python3-numpy python3-tomli python3-venv
        elif [ "${VERSION_ID}" = "20.04" ]; then
            apt install cmake g++ gcc git libasio-dev libelf-dev libepoxy-dev \
                libglib2.0-dev libpixman-1-dev libsdl2-dev \
                libvirglrenderer-dev meson ninja-build ocl-icd-opencl-dev \
                python3 python3-dev python3-numpy python3-pip python3-venv
            pip install --user tomli
        else
            unsupported_operating_system
        fi
    fi
else
    case "$(uname)" in
        Darwin*)
            MAJOR_VERSION=$(sw_vers -productVersion | cut -d "." -f 1)
            readonly MAJOR_VERSION

            if [ "${MAJOR_VERSION}" = "15" ]; then
                brew install asio cmake libelf meson ninja python3 sdl2
                pip install --user numpy
            elif [ "${MAJOR_VERSION}" = "14" ]; then
                brew install asio cmake libelf meson ninja python3 sdl2
                pip install --user numpy
            fi

            brew tap quic/quic https://github.com/quic/homebrew-quic.git
            brew install quic/quic/virglrenderer

            ;;
        *)
            unsupported_operating_system
    esac
fi
