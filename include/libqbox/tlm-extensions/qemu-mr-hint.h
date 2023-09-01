/*
 *  This file is part of libqbox
 *  Copyright (C) 2021  GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_TLM_EXTENSIONS_QEMU_MR_HINT_H
#define _LIBQBOX_TLM_EXTENSIONS_QEMU_MR_HINT_H

#include <tlm>

#include "libqemu-cxx/libqemu-cxx.h"

class QemuMrHintTlmExtension : public tlm::tlm_extension<QemuMrHintTlmExtension>
{
private:
    qemu::MemoryRegion m_mr;
    uint64_t m_offset;

public:
    QemuMrHintTlmExtension() = default;
    QemuMrHintTlmExtension(const QemuMrHintTlmExtension&) = default;

    QemuMrHintTlmExtension(qemu::MemoryRegion mr, uint64_t offset): m_mr(mr), m_offset(offset) {}

    virtual tlm_extension_base* clone() const override { return new QemuMrHintTlmExtension(*this); }

    virtual void copy_from(tlm_extension_base const& ext) override
    {
        m_mr = static_cast<const QemuMrHintTlmExtension&>(ext).m_mr;
        m_offset = static_cast<const QemuMrHintTlmExtension&>(ext).m_offset;
    }

    qemu::MemoryRegion get_mr() const { return m_mr; }
    uint64_t get_offset() const { return m_offset; }
};

#endif
