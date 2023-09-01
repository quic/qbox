/*
 *  This file is part of libqemu-cxx
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libqemu/libqemu.h>

#include "libqemu-cxx/libqemu-cxx.h"
#include "internals.h"

namespace qemu {

RcuReadLock::RcuReadLock(std::shared_ptr<LibQemuInternals> internals): m_int(internals)
{
    m_int->exports().rcu_read_lock();
}

RcuReadLock::~RcuReadLock()
{
    if (m_int) {
        m_int->exports().rcu_read_unlock();
    }
}

RcuReadLock::RcuReadLock(RcuReadLock&& o): m_int(o.m_int) { o.m_int.reset(); }

RcuReadLock& RcuReadLock::operator=(RcuReadLock&& o)
{
    std::swap(m_int, o.m_int);
    return *this;
}

} // namespace qemu
