/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2021 Greensocs
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
} // namespace qemu
