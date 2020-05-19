/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  Clement Deschamps and Luc Michel
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

#include <libqemu/libqemu.h>

#include "libqemu-cxx.h"

namespace qemu {

/* ::MemTxtResult <-> MemoryRegionOps::MemTxResult mapping */
static inline MemoryRegionOps::MemTxResult QEMU_TO_LIB_MEMTXRESULT_MAPPING(uint32_t value)
{
    switch (value) {
    case MEMTX_OK: return MemoryRegionOps::MemTxOK;
    case MEMTX_ERROR: return MemoryRegionOps::MemTxError;
    case MEMTX_DECODE_ERROR: return MemoryRegionOps::MemTxDecodeError;
    }
    return MemoryRegionOps::MemTxError;
}

static inline uint32_t LIB_TO_QEMU_MEMTXRESULT_MAPPING(MemoryRegionOps::MemTxResult value)
{
    switch (value) {
    case MemoryRegionOps::MemTxOK: return MEMTX_OK;
    case MemoryRegionOps::MemTxError: return MEMTX_ERROR;
    case MemoryRegionOps::MemTxDecodeError: return MEMTX_DECODE_ERROR;
    }
    return MEMTX_ERROR;
}

/*
 * ===============
 * MemoryRegionOps
 * ===============
 */

MemoryRegionOps::MemTxAttrs::MemTxAttrs(const ::MemTxAttrs &qemu_attrs)
    : secure(qemu_attrs.secure), exclusive(qemu_attrs.exclusive)
{}

MemoryRegionOps::MemoryRegionOps(QemuMemoryRegionOps *ops, LibQemuExports &exports)
    : m_ops(ops), m_exports(exports)
{}

MemoryRegionOps::~MemoryRegionOps()
{
    m_exports.mr_ops_free(m_ops);
}

static ::MemTxResult generic_read_cb(void *opaque, hwaddr addr, uint64_t *data,
                                     unsigned int size, MemTxAttrs qemu_attrs)
{
    MemoryRegionOps *ops = reinterpret_cast<MemoryRegionOps*>(opaque);
    MemoryRegionOps::MemTxAttrs attrs(qemu_attrs);
    MemoryRegionOps::MemTxResult res;

    res =  ops->get_read_callback()(addr, data, size, attrs);
    return LIB_TO_QEMU_MEMTXRESULT_MAPPING(res);
}

void MemoryRegionOps::set_read_callback(ReadCallback cb)
{
    m_read_cb = cb;
    m_exports.mr_ops_set_read_cb(m_ops, generic_read_cb);
}

static ::MemTxResult generic_write_cb(void *opaque, hwaddr addr, uint64_t data,
                                     unsigned int size, MemTxAttrs qemu_attrs)
{
    MemoryRegionOps *ops = reinterpret_cast<MemoryRegionOps*>(opaque);
    MemoryRegionOps::MemTxAttrs attrs(qemu_attrs);
    MemoryRegionOps::MemTxResult res;

    res = ops->get_write_callback()(addr, data, size, attrs);
    return LIB_TO_QEMU_MEMTXRESULT_MAPPING(res);
}

void MemoryRegionOps::set_write_callback(WriteCallback cb)
{
    m_write_cb = cb;
    m_exports.mr_ops_set_write_cb(m_ops, generic_write_cb);
}

void MemoryRegionOps::set_max_access_size(unsigned size)
{
    m_exports.mr_ops_set_max_access_size(m_ops, size);
}

/*
 * ============
 * MemoryRegion
 * ============
 */

MemoryRegion::~MemoryRegion()
{
    for (auto &mr: m_subregions) {
        internal_del_subregion(mr);
    }

    m_subregions.clear();
}

uint64_t MemoryRegion::get_size()
{
    QemuMemoryRegion *mr = reinterpret_cast<QemuMemoryRegion*>(m_obj);
    return m_exports->memory_region_size(mr);
}

void MemoryRegion::init_io(Object owner, const char *name, uint64_t size, MemoryRegionOpsPtr ops)
{
    QemuMemoryRegion *mr = reinterpret_cast<QemuMemoryRegion*>(m_obj);
    QemuMemoryRegionOps *qemu_ops = ops->get_qemu_mr_ops();

    m_ops = ops;

    m_exports->memory_region_init_io(mr, owner.get_qemu_obj(), qemu_ops, m_ops.get(), name, size);
}

void MemoryRegion::init_ram_ptr(Object owner, const char *name, uint64_t size, void *ptr)
{
    QemuMemoryRegion *mr = reinterpret_cast<QemuMemoryRegion*>(m_obj);

    m_exports->memory_region_init_ram_ptr(mr, owner.get_qemu_obj(), name, size, ptr);
}

void MemoryRegion::init_alias(Object owner, const char *name, MemoryRegion &root,
                              uint64_t offset, uint64_t size)
{
    QemuMemoryRegion *mr = reinterpret_cast<QemuMemoryRegion*>(m_obj);
    QemuMemoryRegion *root_mr = reinterpret_cast<QemuMemoryRegion*>(root.m_obj);

    m_exports->memory_region_init_alias(mr, owner.get_qemu_obj(), name, root_mr,
                                        offset, size);
}

void MemoryRegion::add_subregion(MemoryRegion &mr, uint64_t offset)
{
    QemuMemoryRegion *this_mr = reinterpret_cast<QemuMemoryRegion*>(m_obj);
    QemuMemoryRegion *sub_mr = reinterpret_cast<QemuMemoryRegion*>(mr.m_obj);

    m_exports->memory_region_add_subregion(this_mr, offset, sub_mr);
    m_subregions.insert(mr);
    mr.container = this;
}

void MemoryRegion::internal_del_subregion(const MemoryRegion &mr)
{
    QemuMemoryRegion *this_mr = reinterpret_cast<QemuMemoryRegion*>(m_obj);
    QemuMemoryRegion *sub_mr = reinterpret_cast<QemuMemoryRegion*>(mr.m_obj);

    m_exports->memory_region_del_subregion(this_mr, sub_mr);
}

void MemoryRegion::del_subregion(const MemoryRegion &mr)
{
    internal_del_subregion(mr);
    m_subregions.erase(mr);
}

MemoryRegion::MemTxResult
MemoryRegion::dispatch_read(uint64_t addr, uint64_t *data,
                            uint64_t size, MemTxAttrs attrs)
{
    ::MemTxAttrs qemu_attrs = {};
    ::MemTxResult qemu_res;
    QemuMemoryRegion *mr = reinterpret_cast<QemuMemoryRegion*>(m_obj);

    qemu_attrs.secure = attrs.secure;

    qemu_res = m_exports->memory_region_dispatch_read(mr, addr, data,
                                                      size, qemu_attrs);

    return QEMU_TO_LIB_MEMTXRESULT_MAPPING(qemu_res);
}

MemoryRegion::MemTxResult
MemoryRegion::dispatch_write(uint64_t addr, uint64_t data,
                             uint64_t size, MemTxAttrs attrs)
{
    ::MemTxAttrs qemu_attrs = {};
    ::MemTxResult qemu_res;
    QemuMemoryRegion *mr = reinterpret_cast<QemuMemoryRegion*>(m_obj);

    qemu_attrs.secure = attrs.secure;

    qemu_res = m_exports->memory_region_dispatch_write(mr, addr, data,
                                                       size, qemu_attrs);

    return QEMU_TO_LIB_MEMTXRESULT_MAPPING(qemu_res);
}

};


