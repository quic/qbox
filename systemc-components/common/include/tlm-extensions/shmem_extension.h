/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
    int m_fd;

    ShmemIDExtension(): m_mapped_addr(0), m_size(0){};
    ShmemIDExtension(const ShmemIDExtension&) = default;
    ShmemIDExtension(std::string& s, uint64_t addr, uint64_t size, int fd = -1)
        : m_memid(s), m_mapped_addr(addr), m_size(size), m_fd(fd)
    {
        SCP_DEBUG("ShmemIDExtension") << "ShmemIDExtension constructor";
    }
    ShmemIDExtension& operator=(ShmemIDExtension o)
    {
        m_memid.assign(o.m_memid);
        m_mapped_addr = o.m_mapped_addr;
        m_size = o.m_size;
        m_fd = o.m_fd;
        return *this;
    }

    virtual tlm_extension_base* clone() const override { return const_cast<ShmemIDExtension*>(this); }

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
