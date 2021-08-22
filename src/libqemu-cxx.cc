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
#include <cassert>

#include <libqemu/libqemu.h>

#include "libqemu-cxx/libqemu-cxx.h"
#include "internals.h"

namespace qemu {

LibQemu::LibQemu(LibraryLoaderIface &library_loader, const char *lib_path)
    : m_library_loader(library_loader)
    , m_lib_path(lib_path)
{
}

LibQemu::LibQemu(LibraryLoaderIface &library_loader, Target t)
    : m_library_loader(library_loader)
    , m_lib_path(nullptr)
    , m_target(t)
{
}

LibQemu::~LibQemu()
{
    for (auto s: m_qemu_argv) {
        delete [] s;
    }
}

void LibQemu::push_qemu_arg(const char *arg)
{
    char * dst = new char[std::strlen(arg)+1];
    std::strcpy(dst, arg);

    assert(!is_inited());
    m_qemu_argv.push_back(dst);
}

void LibQemu::push_qemu_arg(std::initializer_list<const char *> args)
{
    for (auto arg: args) {
        push_qemu_arg(arg);
    }
}

void LibQemu::init()
{
    const char *libname;
    LibQemuInitFct qemu_init = nullptr;
    LibQemuExports *exports = nullptr;

    if (m_lib_path == nullptr) {
        /* constructed with a Target */
        libname = get_target_lib(m_target);

        if (libname == nullptr) {
            /*
             * Null libname means libqemu hasn't been compiled with support
             * for this target.
             */
            throw TargetNotSupportedException(m_target);
        }
    } else {
        /* constructed with a lib path */
        libname = m_lib_path;
    }

    m_lib = m_library_loader.load_library(libname);

    if (m_lib == nullptr) {
        throw LibraryLoadErrorException(libname, m_library_loader.get_last_error());
    }

    if (!m_lib->symbol_exists(LIBQEMU_INIT_SYM_STR)) {
        throw InvalidLibraryException(libname, LIBQEMU_INIT_SYM_STR);
    }

    qemu_init = reinterpret_cast<LibQemuInitFct>(m_lib->get_symbol(LIBQEMU_INIT_SYM_STR));
    exports = qemu_init(m_qemu_argv.size(), &m_qemu_argv[0]);

    m_int = std::make_shared<LibQemuInternals>(*this, exports);
    init_callbacks();
}

void LibQemu::start_gdb_server(std::string port)
{
    m_int->exports().gdbserver_start(port.c_str());
}

void LibQemu::vm_start()
{
    m_int->exports().vm_start();
}

void LibQemu::vm_stop_paused()
{
    m_int->exports().vm_stop_paused();
}

int64_t LibQemu::get_virtual_clock()
{
    return m_int->exports().clock_virtual_get_ns();
}

void LibQemu::tb_invalidate_phys_range(uint64_t start, uint64_t end)
{
    m_int->exports().tb_invalidate_phys_range(start, end);
}

QemuObject* LibQemu::object_new_internal(const char *type_name)
{
    QemuObject *o = m_int->exports().object_new(type_name);
    QemuObject *root = m_int->exports().object_get_root();
    QemuError *err = nullptr;

    m_int->exports().object_property_add_child(root, "libqemu-obj[*]", o);

    if (err != nullptr) {
        throw LibQemuException("Error while parenting the new object");
    }

    return o;
}

Object LibQemu::object_new(const char *type_name)
{
    Object o(object_new_internal(type_name), m_int);
    return o;
}

std::shared_ptr<MemoryRegionOps> LibQemu::memory_region_ops_new()
{
    QemuMemoryRegionOps *ops = m_int->exports().mr_ops_new();

    return std::make_shared<MemoryRegionOps>(ops, m_int);
}

std::shared_ptr<AddressSpace> LibQemu::address_space_new()
{
    QemuAddressSpace *as = m_int->exports().address_space_new();

    return std::make_shared<AddressSpace>(as, m_int);
}

std::shared_ptr<AddressSpace> LibQemu::address_space_get_system_memory() {
    QemuAddressSpace *as = m_int->exports().address_space_get_system_memory();
    return std::make_shared<AddressSpace>(as, m_int);
}

static void qemu_gpio_generic_handler(void * opaque, int n, int level)
{
    Gpio::GpioProxy *proxy = reinterpret_cast<Gpio::GpioProxy*>(opaque);

    proxy->event(level);
}

Gpio LibQemu::gpio_new()
{
    std::shared_ptr<Gpio::GpioProxy> proxy = std::make_shared<Gpio::GpioProxy>();

    QemuGpio *qemu_gpio = m_int->exports().gpio_new(qemu_gpio_generic_handler, proxy.get());

    Gpio gpio(Object(reinterpret_cast<QemuObject*>(qemu_gpio), m_int));
    gpio.set_proxy(proxy);

    return gpio;
}

std::shared_ptr<Timer> LibQemu::timer_new()
{
    return std::make_shared<Timer>(m_int);
}

Chardev LibQemu::chardev_new(const char *label, const char *type)
{
    QemuChardev *qemu_char_dev = m_int->exports().char_dev_new(label, type);
    Chardev ret(Object(reinterpret_cast<QemuObject *>(qemu_char_dev), m_int));

    return ret;
}

void LibQemu::check_cast(Object &o, const char *type)
{
    /* TODO */
}

void LibQemu::lock_iothread()
{
    m_int->exports().qemu_mutex_lock_iothread();
}

void LibQemu::unlock_iothread()
{
    m_int->exports().qemu_mutex_unlock_iothread();
}

void LibQemu::coroutine_yield()
{
    m_int->exports().coroutine_yield();
}

}
