#!/usr/bin/bash
#
# This file is part of libqbox
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#


if [ -e /run/systemd/resolve/resolv.conf ]; then
    mv /etc/resolv.conf /etc/resolv.conf.old
    ln -s /run/systemd/resolve/resolv.conf /etc/resolv.conf
fi
