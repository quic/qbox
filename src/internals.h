/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2021 Luc Michel
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

#ifndef _LIBQEMU_CXX_INTERNALS_
#define _LIBQEMU_CXX_INTERNALS_

#include "libqemu/libqemu.h"
#include "libqemu-cxx/libqemu-cxx.h"

namespace qemu {

class LibQemuInternals {
private:
    LibQemu &m_inst;
    LibQemuExports *m_exports;

public:
    LibQemuInternals(LibQemu &inst, LibQemuExports *exports)
        : m_inst(inst)
        , m_exports(exports)
    {}

    const LibQemuExports & exports() const { return *m_exports; };
    LibQemu & get_inst() { return m_inst; }
};

}

#endif
