/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2015-2019  GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdexcept>

#include <libqemu-cxx/target_info.h>

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
        : LibQemuException(std::string("target `") + get_target_name(t) + "` disabled at compile time.")
    {
    }

    virtual ~TargetNotSupportedException() throw() {}
};

class LibraryLoadErrorException : public LibQemuException
{
public:
    LibraryLoadErrorException(const char* lib_name, const char* error)
        : LibQemuException(std::string("error while loading libqemu library `") + lib_name + "`: " + error)
    {
    }

    virtual ~LibraryLoadErrorException() throw() {}
};

class InvalidLibraryException : public LibQemuException
{
public:
    InvalidLibraryException(const char* lib_name, const char* symbol)
        : LibQemuException(std::string("Invalid libqemu library `") + lib_name + "` " + "(Symbol `" + symbol +
                           "` not found).")
    {
    }

    virtual ~InvalidLibraryException() throw() {}
};

class SetPropertyException : public LibQemuException
{
public:
    SetPropertyException(const char* type, const char* name, const char* err)
        : LibQemuException(std::string("Error while setting ") + type + " property `" + name + "` on object: " + err)
    {
    }

    virtual ~SetPropertyException() throw() {}
};

class GetPropertyException : public LibQemuException
{
public:
    GetPropertyException(const char* type, const char* name, const char* err)
        : LibQemuException(std::string("Error while getting ") + type + " property `" + name + "` on object: " + err)
    {
    }

    virtual ~GetPropertyException() throw() {}
};

} // namespace qemu
