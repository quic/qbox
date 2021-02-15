/*
 *  This file is part of libqemu-cxx
 *  Copyright (c) 2021 Luc Michel
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

#pragma once

#include <memory>

namespace qemu {
class LibraryIface {
public:
    virtual bool symbol_exists(const char *symbol) = 0;
    virtual void * get_symbol(const char *symbol) = 0;
};

class LibraryLoaderIface {
public:
    using LibraryIfacePtr = std::shared_ptr<LibraryIface>;

    virtual LibraryIfacePtr load_library(const char *lib_name) = 0;
    virtual const char *get_lib_ext() = 0;
};

LibraryLoaderIface * get_default_lib_loader();

}
