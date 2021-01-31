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

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <set>
#include <vector>

#include "exceptions.h"

/* libqemu types forward declaration */
struct LibQemuExports;
struct QemuObject;
struct MemTxAttrs;
struct QemuMemoryRegionOps;
struct QemuTimer;

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
};

class Object;
class MemoryRegionOps;
class Gpio;
class Timer;
class Bus;
class Chardev;

class LibQemu {
private:
    LibraryLoaderIface &m_library_loader;
    std::vector<char *> m_qemu_argv;

    LibQemuExports *m_qemu_exports = nullptr;
    LibraryLoaderIface::LibraryIfacePtr m_lib;

    QemuObject* object_new_internal(const char *type_name);

    void check_cast(Object &o, const char *type);

public:
    LibQemu(LibraryLoaderIface &library_loader);
    ~LibQemu();

    void push_qemu_arg(const char *arg);
    void push_qemu_arg(std::initializer_list<const char *> args);

    void init(const char *lib_path);
    bool is_inited() const { return m_lib != nullptr; }

    /* QEMU GDB stub
     * @port: port the gdb server will be listening on. (ex: "tcp::1234") */
    void start_gdb_server(std::string port);

    void resume_all_vcpus();

    void lock_iothread();
    void unlock_iothread();

    template <class T>
    T object_new() {
        T o(Object(object_new_internal(T::TYPE), *m_qemu_exports));
        check_cast(o, T::TYPE);

        return o;
    }

    int64_t get_virtual_clock();

    Object object_new(const char *type_name);
    std::shared_ptr<MemoryRegionOps> memory_region_ops_new();
    Gpio gpio_new();

    std::shared_ptr<Timer> timer_new();

    Chardev chardev_new(const char *label, const char *type);

    void tb_invalidate_phys_range(uint64_t start, uint64_t end);
};

class Object {
protected:
    QemuObject *m_obj = nullptr;
    LibQemuExports *m_exports = nullptr;

    bool check_cast_by_type(const char *type_name) const
    {
        return true;
    }


public:
    Object() = default;
    Object(QemuObject *obj, LibQemuExports &exports);
    Object(const Object &o);
    Object(Object &&o);

    Object & operator=(Object o);

    virtual ~Object();

    bool valid() const { return m_obj != nullptr; }

    void set_prop_bool(const char *name, bool val);
    void set_prop_int(const char *name, int64_t val);
    void set_prop_str(const char *name, const char *val);
    void set_prop_link(const char *name, const Object &link);

    QemuObject *get_qemu_obj() { return m_obj; }

    uintptr_t get_inst_id() const { return reinterpret_cast<uintptr_t>(m_exports); }

    template <class T>
    bool check_cast() const { return check_cast_by_type(T::TYPE); }
};

class Gpio : public Object {
public:
    typedef std::function<void (bool)> GpioEventFn;

    class GpioProxy {
    protected:
        bool m_prev_valid = false;
        bool m_prev;
        GpioEventFn m_cb;

    public:
        void event(bool level)
        {
            if (!m_prev_valid || (level != m_prev)) {
                m_cb(level);
            }

            m_prev_valid = true;
            m_prev = level;
        }

        void set_callback(GpioEventFn cb) { m_cb = cb; }
    };

private:
    std::shared_ptr<GpioProxy> m_proxy;

public:
    static constexpr const char * const TYPE = "irq";

    Gpio() = default;
    Gpio(const Gpio &o) = default;
    Gpio(const Object &o) : Object(o) {}

    void set(bool lvl);

    void set_proxy(std::shared_ptr<GpioProxy> proxy) { m_proxy = proxy; }

    void set_event_callback(GpioEventFn cb)
    {
        if (m_proxy) {
            m_proxy->set_callback(cb);
        }
    }
};

class MemoryRegionOps {
public:
    enum MemTxResult {
        MemTxOK,
        MemTxError,
        MemTxDecodeError,
        MemTxOKExitTB
    };

    struct MemTxAttrs {
        bool secure = false;
        bool exclusive = false;
        bool debug = false;

        MemTxAttrs() = default;
        MemTxAttrs(const ::MemTxAttrs &qemu_attrs);
    };

    typedef std::function<MemTxResult (uint64_t, uint64_t *,
                                       unsigned int, MemTxAttrs)> ReadCallback;

    typedef std::function<MemTxResult (uint64_t, uint64_t,
                                       unsigned int, MemTxAttrs)> WriteCallback;

private:
    QemuMemoryRegionOps *m_ops;
    LibQemuExports &m_exports;

    ReadCallback m_read_cb;
    WriteCallback m_write_cb;

public:
    MemoryRegionOps(QemuMemoryRegionOps *ops, LibQemuExports &exports);
    ~MemoryRegionOps();

    void set_read_callback(ReadCallback cb);
    void set_write_callback(WriteCallback cb);

    void set_max_access_size(unsigned size);

    ReadCallback get_read_callback() { return m_read_cb; }
    WriteCallback get_write_callback() { return m_write_cb; }

    QemuMemoryRegionOps *get_qemu_mr_ops() { return m_ops; }
};

typedef std::shared_ptr<MemoryRegionOps> MemoryRegionOpsPtr;

class MemoryRegion : public Object {
private:
    MemoryRegionOpsPtr m_ops;
    std::set<MemoryRegion> m_subregions;

    void internal_del_subregion(const MemoryRegion &mr);

public:
    using MemTxResult = MemoryRegionOps::MemTxResult;
    using MemTxAttrs = MemoryRegionOps::MemTxAttrs;

    static constexpr const char * const TYPE = "qemu:memory-region";

    MemoryRegion *container;

    MemoryRegion() = default;
    MemoryRegion(const MemoryRegion &) = default;
    MemoryRegion(const Object &o) : Object(o) {}

    ~MemoryRegion();

    uint64_t get_size();

    void init_io(Object owner, const char *name, uint64_t size, MemoryRegionOpsPtr ops);
    void init_ram_ptr(Object owner, const char *name, uint64_t size, void *ptr);
    void init_alias(Object owner, const char *name, MemoryRegion &root,
                    uint64_t offset, uint64_t size);

    void add_subregion(MemoryRegion &mr, uint64_t offset);
    void del_subregion(const MemoryRegion &mr);

    MemTxResult dispatch_read(uint64_t addr, uint64_t *data,
                              uint64_t size, MemTxAttrs attrs);

    MemTxResult dispatch_write(uint64_t addr, uint64_t data,
                               uint64_t size, MemTxAttrs attrs);

    bool operator< (const MemoryRegion &mr) const { return m_obj < mr.m_obj; }
};

class Device : public Object {
public:
    static constexpr const char * const TYPE = "device";

    Device() = default;
    Device(const Device &) = default;
    Device(const Object &o) : Object(o) {}

    void connect_gpio_out(int idx, Gpio gpio);
    void connect_gpio_out_named(const char *name, int idx, Gpio gpio);

    Gpio get_gpio_in(int idx);
    Gpio get_gpio_in_named(const char *name, int idx);

    Bus get_child_bus(const char *name);
    void set_parent_bus(Bus bus);

    void set_prop_chardev(const char *name, Chardev chr);
};

class SysBusDevice : public Device {
public:
    static constexpr const char * const TYPE = "sys-bus-device";

    SysBusDevice() = default;
    SysBusDevice(const SysBusDevice &) = default;
    SysBusDevice(const Object &o) : Device(o) {}

    MemoryRegion mmio_get_region(int id);

    void connect_gpio_out(int idx, Gpio gpio);
};

class Cpu : public Device {
public:
    static constexpr const char * const TYPE = "cpu";

    Cpu() = default;
    Cpu(const Cpu &) = default;
    Cpu(const Object &o) : Device(o) {}

    void loop();
    bool loop_is_busy();

    bool can_run();

    void halt(bool halted);
    void reset();

    void register_thread();

    Cpu set_as_current();

    void kick();

    void request_exit();

    void async_safe_run(void (*handler)(void *), void *arg);
};

class Timer {
public:
    typedef std::function<void ()> TimerCallbackFn;

private:
    LibQemuExports &m_exports;
    QemuTimer *m_timer = nullptr;
    TimerCallbackFn m_cb;

public:
    Timer(LibQemuExports &exports);
    ~Timer();

    void set_callback(TimerCallbackFn cb);

    void mod(int64_t deadline);
    void del();

};

class Bus : public Object {
public:
    static constexpr const char * const TYPE = "bus";

    Bus() = default;
    Bus(const Bus &o) = default;
    Bus(const Object &o) : Object(o) {}
};

class Chardev : public Object {
public:
    static constexpr const char * const TYPE = "chardev";

    Chardev() = default;
    Chardev(const Chardev &o) = default;
    Chardev(const Object &o) : Object(o) {}
};

}; /* namespace qemu */
