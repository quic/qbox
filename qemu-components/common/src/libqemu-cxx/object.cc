/*
 * This file is part of libqemu-cxx
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libqemu-cxx/libqemu-cxx.h>
#include <internals.h>

namespace qemu {

Object::Object(QemuObject* obj, std::shared_ptr<LibQemuInternals>& internals): m_obj(obj), m_int(internals)
{
    m_int->exports().object_ref(obj);
}

Object::Object(const Object& o): m_obj(o.m_obj), m_int(o.m_int)
{
    if (valid()) {
        m_int->exports().object_ref(m_obj);
    }
}

Object::Object(Object&& o): m_obj(o.m_obj), m_int(o.m_int) { o.m_obj = nullptr; }

Object& Object::operator=(Object o)
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

LibQemu& Object::get_inst() { return m_int->get_inst(); }

void Object::set_prop_bool(const char* name, bool val)
{
    QemuError* e = nullptr;
    m_int->exports().object_property_set_bool(m_obj, name, val, &e);

    if (e != nullptr) {
        throw SetPropertyException("bool", name, m_int->exports().error_get_pretty(e));
    }
}

void Object::set_prop_int(const char* name, int64_t val)
{
    QemuError* e = nullptr;
    m_int->exports().object_property_set_int(m_obj, name, val, &e);

    if (e != nullptr) {
        throw SetPropertyException("int", name, m_int->exports().error_get_pretty(e));
    }
}

void Object::set_prop_uint(const char* name, uint64_t val)
{
    QemuError* e = nullptr;
    m_int->exports().object_property_set_uint(m_obj, name, val, &e);

    if (e != nullptr) {
        throw SetPropertyException("int", name, m_int->exports().error_get_pretty(e));
    }
}

void Object::set_prop_str(const char* name, const char* val)
{
    QemuError* e = nullptr;
    m_int->exports().object_property_set_str(m_obj, name, val, &e);

    if (e != nullptr) {
        throw SetPropertyException("str", name, m_int->exports().error_get_pretty(e));
    }
}

void Object::set_prop_link(const char* name, const Object& link)
{
    QemuError* e = nullptr;
    m_int->exports().object_property_set_link(m_obj, name, link.m_obj, &e);

    if (e != nullptr) {
        throw SetPropertyException("link", name, m_int->exports().error_get_pretty(e));
    }
}

void Object::set_prop_parse(const char* name, const char* value)
{
    QemuError* e = nullptr;
    m_int->exports().object_property_parse(m_obj, name, value, &e);

    if (e != nullptr) {
        throw SetPropertyException("parse", name, m_int->exports().error_get_pretty(e));
    }
}

Object Object::get_prop_link(const char* name)
{
    QemuError* e = nullptr;
    QemuObject* obj = nullptr;
    obj = m_int->exports().object_property_get_link(m_obj, name, &e);

    if (e != nullptr) {
        throw GetPropertyException("link", name, m_int->exports().error_get_pretty(e));
    }

    return Object(obj, m_int);
}

void Object::clear_callbacks() { m_int->clear_callbacks(*this); }

} // namespace qemu
