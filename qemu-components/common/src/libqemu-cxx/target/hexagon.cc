/*
 * This file is part of libqemu-cxx
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libqemu/libqemu.h>

#include <libqemu-cxx/target/hexagon.h>
#include "internals.h"

namespace qemu {
void CpuHexagon::register_reset() { m_int->exports().cpu_hexagon_register_reset(m_obj); }
}
