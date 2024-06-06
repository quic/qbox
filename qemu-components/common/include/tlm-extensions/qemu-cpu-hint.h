/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_TLM_EXTENSIONS_QEMU_CPU_HINT_H
#define _LIBQBOX_TLM_EXTENSIONS_QEMU_CPU_HINT_H

#include <tlm>

#include <libqemu-cxx/libqemu-cxx.h>

class QemuCpuHintTlmExtension : public tlm::tlm_extension<QemuCpuHintTlmExtension>
{
private:
    qemu::Cpu m_cpu;

public:
    QemuCpuHintTlmExtension() = default;
    QemuCpuHintTlmExtension(const QemuCpuHintTlmExtension&) = default;

    QemuCpuHintTlmExtension(qemu::Cpu cpu): m_cpu(cpu) {}

    virtual tlm_extension_base* clone() const override { return new QemuCpuHintTlmExtension(*this); }

    virtual void copy_from(tlm_extension_base const& ext) override
    {
        m_cpu = static_cast<const QemuCpuHintTlmExtension&>(ext).m_cpu;
    }

    void set_cpu(qemu::Cpu cpu) { m_cpu = cpu; }
    qemu::Cpu get_cpu() const { return m_cpu; }
};

#endif
