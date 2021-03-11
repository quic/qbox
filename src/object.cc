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

#include "libqemu-cxx/libqemu-cxx.h"
#include "internals.h"

namespace qemu {

Object::Object(QemuObject *obj, std::shared_ptr<LibQemuInternals> &internals)
        : m_obj(obj)
        , m_int(internals)
{
    m_int->exports().object_ref(obj);
}

Object::Object(const Object &o)
    : m_obj(o.m_obj)
    , m_int(o.m_int)
{
    if (valid()) {
        m_int->exports().object_ref(m_obj);
    }
}

Object::Object(Object &&o)
    : m_obj(o.m_obj)
    , m_int(o.m_int)
{
    o.m_obj = nullptr;
}

Object & Object::operator=(Object o)
{
    if (this != &o) {
        std::swap(m_obj, o.m_obj);
        std::swap(m_int, o.m_int);
    }

    return *this;
}

Object::~Object()
{
    if (valid()) {
        m_int->exports().object_unref(m_obj);
    }
}

LibQemu & Object::get_inst()
{
    return m_int->get_inst();
}

void Object::set_prop_bool(const char *name, bool val)
{
    QemuError *e = nullptr;
    m_int->exports().object_property_set_bool(m_obj, name, val, &e);

    if (e != nullptr) {
        throw SetPropertyException("bool", name);
    }
}

void Object::set_prop_int(const char *name, int64_t val)
{
    QemuError *e = nullptr;
    m_int->exports().object_property_set_int(m_obj, name, val, &e);

    if (e != nullptr) {
        throw SetPropertyException("int", name);
    }
}

void Object::set_prop_str(const char *name, const char *val)
{
    QemuError *e = nullptr;
    m_int->exports().object_property_set_str(m_obj, name, val, &e);

    if (e != nullptr) {
        throw SetPropertyException("str", name);
    }
}

void Object::set_prop_link(const char *name, const Object &link)
{
    QemuError *e = nullptr;
    m_int->exports().object_property_set_link(m_obj, name, link.m_obj, &e);

    if (e != nullptr) {
        throw SetPropertyException("link", name);
    }
}

}
