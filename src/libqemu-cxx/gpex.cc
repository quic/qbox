/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2021 Greensocs
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

#include "libqemu-cxx/libqemu-cxx.h"
#include "internals.h"

namespace qemu {

void GpexHost::set_irq_num(int idx, int gic_irq)
{
    QemuSysBusDevice* qemu_sbd;

    qemu_sbd = reinterpret_cast<QemuSysBusDevice*>(m_obj);

    m_int->exports().gpex_set_irq_num(qemu_sbd, idx, gic_irq);
}

} // namespace qemu
