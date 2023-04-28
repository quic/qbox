/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef _GREENSOCS_SHMEMID_EXTENSION_H
#define _GREENSOCS_SHMEMID_EXTENSION_H

#include <systemc>
#include <tlm>

namespace gs {

/**
 * @class Shared memory ID extension
 *
 * @details This ID field is expected to be held by the component offering, and provided to other
 * components The extension is therefore held and 'memory managed' by the owner. It should not be
 * deleted. It may be 'shared' multiple times.
 */

class ShmemIDExtension : public tlm::tlm_extension<ShmemIDExtension>
{
public:
    std::string m_memid;
    uint64_t m_mapped_addr;
    uint64_t m_size;

    ShmemIDExtension(): m_mapped_addr(0), m_size(0){};
    ShmemIDExtension(const ShmemIDExtension&) = default;
    ShmemIDExtension(std::string& s, uint64_t addr, uint64_t size)
        : m_memid(s), m_mapped_addr(addr), m_size(size)
    {
        SCP_DEBUG("ShmemIDExtension") << "ShmemIDExtension constructor";
    }
    ShmemIDExtension& operator=(ShmemIDExtension o)
    {
        m_memid.assign(o.m_memid);
        m_mapped_addr = o.m_mapped_addr;
        m_size = o.m_size;
        return *this;
    }

    virtual tlm_extension_base* clone() const override
    {
        return const_cast<ShmemIDExtension*>(this);
    }

    virtual void copy_from(const tlm_extension_base& ext) override
    {
        const ShmemIDExtension& other = static_cast<const ShmemIDExtension&>(ext);
        *this = other;
    }

    virtual void free() override { return; } // we will not free this extension

    bool empty() { return m_size == 0; }
};
} // namespace gs
#endif
