/*
 * This file is part of libqemu-cxx
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libqemu-cxx/libqemu-cxx.h>
#include <internals.h>

namespace qemu {

static void generic_cpu_end_of_loop_cb(QemuObject* cpu, void* opaque)
{
    LibQemuInternals* internals = reinterpret_cast<LibQemuInternals*>(opaque);

    internals->get_cpu_end_of_loop_cb().call(cpu);
}

static void generic_cpu_kick_cb(QemuObject* cpu, void* opaque)
{
    LibQemuInternals* internals = reinterpret_cast<LibQemuInternals*>(opaque);

    internals->get_cpu_kick_cb().call(cpu);
}

static IOMMUMemoryRegion::IOMMUTLBEntry generic_iommu_translate_cb(QemuObject* mr, void* opaque, uint64_t addr,
                                                                   IOMMUMemoryRegion::IOMMUAccessFlags flag, int idx)
{
    LibQemuInternals* internals = reinterpret_cast<LibQemuInternals*>(opaque);
    IOMMUMemoryRegion::IOMMUTLBEntry entry;
    internals->get_iommu_translate_cb().call(mr, &entry, addr, flag, idx);
    return entry;
}
static void generic_cpu_riscv_mip_update_cb(QemuObject* cpu, uint32_t value, void* opaque)
{
    LibQemuInternals* internals = reinterpret_cast<LibQemuInternals*>(opaque);

    internals->get_cpu_riscv_mip_update_cb().call(cpu, value);
}

void LibQemu::init_callbacks()
{
    m_int->exports().set_cpu_end_of_loop_cb(generic_cpu_end_of_loop_cb, m_int.get());

    m_int->exports().set_cpu_kick_cb(generic_cpu_kick_cb, m_int.get());

    m_int->exports().set_iommu_translate_cb((LibQemuIOMMUTranslateFn)generic_iommu_translate_cb, m_int.get());

    if (m_int->exports().cpu_riscv_register_mip_update_callback != nullptr) {
        m_int->exports().cpu_riscv_register_mip_update_callback(generic_cpu_riscv_mip_update_cb, m_int.get());
    }
}

} // namespace qemu
