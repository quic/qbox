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

#include <cstring>

#include <libqemu/libqemu.h>

#include "libqemu-cxx.h"

namespace qemu {

LibQemu::LibQemu(LibraryLoaderIface &library_loader)
    : m_library_loader(library_loader)
{
}

LibQemu::~LibQemu()
{
    for (auto s: m_qemu_argv) {
        delete s;
    }
}

void LibQemu::push_qemu_arg(const char *arg)
{
    char * dst = new char[std::strlen(arg)+1];
    std::strcpy(dst, arg);

    m_qemu_argv.push_back(dst);
}

void LibQemu::push_qemu_arg(std::initializer_list<const char *> args)
{
    for (auto arg: args) {
        push_qemu_arg(arg);
    }
}

void LibQemu::init(const char *libname)
{
    LibQemuInitFct qemu_init = nullptr;

    m_lib = m_library_loader.load_library(libname);

    if (m_lib == nullptr) {
        throw LibraryNotFoundException(libname);
    }

    if (!m_lib->symbol_exists(LIBQEMU_INIT_SYM_STR)) {
        throw InvalidLibraryException(libname, LIBQEMU_INIT_SYM_STR);
    }

    qemu_init = reinterpret_cast<LibQemuInitFct>(m_lib->get_symbol(LIBQEMU_INIT_SYM_STR));

    m_qemu_exports = qemu_init(m_qemu_argv.size(), &m_qemu_argv[0]);
}

void LibQemu::start_gdb_server(std::string port)
{
    m_qemu_exports->gdbserver_start(port.c_str());
}

int64_t LibQemu::get_virtual_clock()
{
    return m_qemu_exports->clock_virtual_get_ns();
}

void LibQemu::tb_invalidate_phys_range(uint64_t start, uint64_t end)
{
    m_qemu_exports->tb_invalidate_phys_range(start, end);
}

QemuObject* LibQemu::object_new_internal(const char *type_name)
{
    QemuObject *o = m_qemu_exports->object_new(type_name);
    QemuObject *root = m_qemu_exports->object_get_root();
    QemuError *err = nullptr;

    m_qemu_exports->object_property_add_child(root, "libqemu-obj[*]", o, &err);

    if (err != nullptr) {
        throw LibQemuException("Error while parenting the new object");
    }

    return o;
}

Object LibQemu::object_new(const char *type_name)
{
    Object o(object_new_internal(type_name), *m_qemu_exports);
    return o;
}

std::shared_ptr<MemoryRegionOps> LibQemu::memory_region_ops_new()
{
    QemuMemoryRegionOps *ops = m_qemu_exports->mr_ops_new();

    return std::make_shared<MemoryRegionOps>(ops, *m_qemu_exports);
}

static void qemu_gpio_generic_handler(void * opaque, int n, int level)
{
    Gpio::GpioProxy *proxy = reinterpret_cast<Gpio::GpioProxy*>(opaque);

    proxy->event(level);
}

Gpio LibQemu::gpio_new()
{
    std::shared_ptr<Gpio::GpioProxy> proxy = std::make_shared<Gpio::GpioProxy>();

    QemuGpio *qemu_gpio = m_qemu_exports->gpio_new(qemu_gpio_generic_handler, proxy.get());

    Gpio gpio(Object(reinterpret_cast<QemuObject*>(qemu_gpio), *m_qemu_exports));
    gpio.set_proxy(proxy);

    return gpio;
}

std::shared_ptr<Timer> LibQemu::timer_new()
{
    return std::make_shared<Timer>(*m_qemu_exports);
}

void LibQemu::check_cast(Object &o, const char *type)
{
    /* TODO */
}

void LibQemu::lock_iothread()
{
    m_qemu_exports->qemu_mutex_lock_iothread();
}

void LibQemu::unlock_iothread()
{
    m_qemu_exports->qemu_mutex_unlock_iothread();
}

Object::Object(QemuObject *obj, LibQemuExports &exports)
        : m_obj(obj)
        , m_exports(&exports)
{
    m_exports->object_ref(obj);
}

Object::Object(const Object &o)
    : m_obj(o.m_obj)
    , m_exports(o.m_exports)
{
    if (valid()) {
        m_exports->object_ref(m_obj);
    }
}

Object::Object(Object &&o)
    : m_obj(o.m_obj)
    , m_exports(o.m_exports)
{
    o.m_obj = nullptr;
}

Object & Object::operator=(Object o)
{
    if (this != &o) {
        std::swap(m_obj, o.m_obj);
        std::swap(m_exports, o.m_exports);
    }

    return *this;
}

Object::~Object()
{
    if (valid()) {
        m_exports->object_unref(m_obj);
    }
}

void Object::set_prop_bool(const char *name, bool val)
{
    QemuError *e = nullptr;
    m_exports->object_property_set_bool(m_obj, val, name, &e);

    if (e != nullptr) {
        throw SetPropertyException("bool", name);
    }
}

void Object::set_prop_int(const char *name, int64_t val)
{
    QemuError *e = nullptr;
    m_exports->object_property_set_int(m_obj, val, name, &e);

    if (e != nullptr) {
        throw SetPropertyException("int", name);
    }
}

void Object::set_prop_str(const char *name, const char *val)
{
    QemuError *e = nullptr;
    m_exports->object_property_set_str(m_obj, val, name, &e);

    if (e != nullptr) {
        throw SetPropertyException("str", name);
    }
}

void Object::set_prop_link(const char *name, const Object &link)
{
    QemuError *e = nullptr;
    m_exports->object_property_set_link(m_obj, link.m_obj, name, &e);

    if (e != nullptr) {
        throw SetPropertyException("link", name);
    }
}

};
