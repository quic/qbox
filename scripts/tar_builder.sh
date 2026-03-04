#!/bin/bash

# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause

if [ -d $1/Packages/libqemu/ ]; then
    echo "Directory libqemu exist"
    cd libqemu/*
    git submodule init
    git submodule update
fi
