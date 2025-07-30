/*
 * This file is part of libqemu-cxx
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libqemu/libqemu.h>

#include <libqemu-cxx/target/riscv.h>
#include <internals.h>

namespace qemu {
void CpuRiscv::set_mip_update_callback(MipUpdateCallbackFn cb)
{
    m_int->get_cpu_riscv_mip_update_cb().register_cb(*this, cb);
}

void CpuRiscv::register_reset() { m_int->exports().cpu_riscv_register_reset(m_obj); }
} // namespace qemu
