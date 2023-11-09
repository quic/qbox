/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "libqbox/components/cpu/cpu.h"

class QemuCpuArm : public QemuCpu
{
public:
    static constexpr qemu::Target ARCH = qemu::Target::AARCH64;

    QemuCpuArm(const sc_core::sc_module_name& name, QemuInstance& inst, const std::string& type_name)
        : QemuCpu(name, inst, type_name)
    {
    }

    /// @returns a new `CpuAarch64` wrapper around `m_cpu`. While this looks like
    /// a copy, it behaves like a downcast. The underlying qemu object reference
    /// count is incremented and decremented correctly by the destructor.
    qemu::CpuAarch64 get_cpu_aarch64() const { return qemu::CpuAarch64(m_cpu); }

    void end_of_elaboration() override
    {
        QemuCpu::end_of_elaboration();

        // This is needed for KVM otherwise the GIC won't reset properly when a system reset is requested
        get_cpu_aarch64().register_reset();
    }

    void start_of_simulation() override
    {
        // According to qemu/hw/arm/virt.c:
        // virt_cpu_post_init() must be called after the CPUs have been realized
        // and the GIC has been created.
        // I am not sure if this the right place where to call it. I wonder if a
        // `before_start_of_simulation` stage would be a better place for this.
        // It MUST be called before `QemuCpu::start_of_simulation()`.
        get_cpu_aarch64().post_init();

        QemuCpu::start_of_simulation();
    }
};
