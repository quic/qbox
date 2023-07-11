/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  GreenSocs
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

#include <stdexcept>

#include "target_info.h"

namespace qemu {

class LibQemuException : public std::runtime_error
{
public:
    LibQemuException(const char* what): std::runtime_error(what) {}
    LibQemuException(const std::string& what): std::runtime_error(what) {}

    virtual ~LibQemuException() throw() {}
};

class TargetNotSupportedException : public LibQemuException
{
public:
    TargetNotSupportedException(Target t)
        : LibQemuException(std::string("target `") + get_target_name(t) +
                           "` disabled at compile time.") {}

    virtual ~TargetNotSupportedException() throw() {}
};

class LibraryLoadErrorException : public LibQemuException
{
public:
    LibraryLoadErrorException(const char* lib_name, const char* error)
        : LibQemuException(std::string("error while loading libqemu library `") + lib_name +
                           "`: " + error) {}

    virtual ~LibraryLoadErrorException() throw() {}
};

class InvalidLibraryException : public LibQemuException
{
public:
    InvalidLibraryException(const char* lib_name, const char* symbol)
        : LibQemuException(std::string("Invalid libqemu library `") + lib_name + "` " +
                           "(Symbol `" + symbol + "` not found).") {}

    virtual ~InvalidLibraryException() throw() {}
};

class SetPropertyException : public LibQemuException
{
public:
    SetPropertyException(const char* type, const char* name, const char* err)
        : LibQemuException(std::string("Error while setting ") + type + " property `" + name +
                           "` on object: " + err) {}

    virtual ~SetPropertyException() throw() {}
};

class GetPropertyException : public LibQemuException
{
public:
    GetPropertyException(const char* type, const char* name, const char* err)
        : LibQemuException(std::string("Error while getting ") + type + " property `" + name +
                           "` on object: " + err) {}

    virtual ~GetPropertyException() throw() {}
};

} // namespace qemu
