/*
 *  This file is part of libqbox
 *  Copyright (C) 2021  Luc Michel
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LIBQBOX_TLM_EXTENSIONS_QEMU_CPU_HINT_H
#define _LIBQBOX_TLM_EXTENSIONS_QEMU_CPU_HINT_H

#include <tlm>

#include "libqemu-cxx/libqemu-cxx.h"

class QemuCpuHintTlmExtension
    : public tlm::tlm_extension<QemuCpuHintTlmExtension>
{
private:
    qemu::Cpu m_cpu;

public:
    QemuCpuHintTlmExtension() = default;
    QemuCpuHintTlmExtension(const QemuCpuHintTlmExtension &) = default;

    QemuCpuHintTlmExtension(qemu::Cpu cpu)
        : m_cpu(cpu)
    {}

    virtual tlm_extension_base* clone() const override
    {
        return new QemuCpuHintTlmExtension(*this);
    }

    virtual void copy_from(tlm_extension_base const &ext) override
    {
        m_cpu = static_cast<const QemuCpuHintTlmExtension&>(ext).m_cpu;
    }

    void set_cpu(qemu::Cpu cpu) { m_cpu = cpu; }
    qemu::Cpu get_cpu() const { return m_cpu; }
};

#endif
