/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBQBOX_QEMU_EXCEPTIONS_H_
#define LIBQBOX_QEMU_EXCEPTIONS_H_

#include <stdexcept>

class QboxException : public std::runtime_error
{
public:
    QboxException(const char* what): std::runtime_error(what) {}

    virtual ~QboxException() throw() {}
};

#endif
