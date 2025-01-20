/*
 * This file is part of libqemu-cxx
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2015-2019
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <cassert>

#include <libqemu/libqemu.h>

#include <libqemu-cxx/libqemu-cxx.h>
#include <internals.h>

namespace qemu {

LibQemu::LibQemu(LibraryLoaderIface& library_loader, const char* lib_path)
    : m_library_loader(library_loader), m_lib_path(lib_path), m_auto_start(true)
{
}

LibQemu::LibQemu(LibraryLoaderIface& library_loader, Target t)
    : m_library_loader(library_loader), m_lib_path(nullptr), m_target(t), m_auto_start(true)
{
}

LibQemu::~LibQemu()
{
    for (auto s : m_qemu_argv) {
        delete[] s;
    }
}

void LibQemu::push_qemu_arg(const char* arg)
{
    char* dst = new char[std::strlen(arg) + 1];
    std::strcpy(dst, arg);
    if (std::strncmp(arg, "-S", 2) == 0) {
        m_auto_start = false;
    }

    assert(!is_inited());
    m_qemu_argv.push_back(dst);
}

void LibQemu::push_qemu_arg(std::initializer_list<const char*> args)
{
    for (auto arg : args) {
        push_qemu_arg(arg);
    }
}

void LibQemu::init()
{
    const char* libname;
    LibQemuInitFct qemu_init = nullptr;
    LibQemuExports* exports = nullptr;

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

    p_api.qemu_plugin_uninstall = reinterpret_cast<qemu_plugin_uninstall_fn>(
        m_lib->get_symbol("qemu_plugin_uninstall"));
    p_api.qemu_plugin_reset = reinterpret_cast<qemu_plugin_reset_fn>(m_lib->get_symbol("qemu_plugin_reset"));
    p_api.qemu_plugin_register_vcpu_init_cb = reinterpret_cast<qemu_plugin_register_vcpu_init_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_init_cb"));
    p_api.qemu_plugin_register_vcpu_exit_cb = reinterpret_cast<qemu_plugin_register_vcpu_exit_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_exit_cb"));
    p_api.qemu_plugin_register_vcpu_idle_cb = reinterpret_cast<qemu_plugin_register_vcpu_idle_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_idle_cb"));
    p_api.qemu_plugin_register_vcpu_resume_cb = reinterpret_cast<qemu_plugin_register_vcpu_resume_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_resume_cb"));
    p_api.qemu_plugin_register_vcpu_tb_trans_cb = reinterpret_cast<qemu_plugin_register_vcpu_tb_trans_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_tb_trans_cb"));
    p_api.qemu_plugin_register_vcpu_tb_exec_cb = reinterpret_cast<qemu_plugin_register_vcpu_tb_exec_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_tb_exec_cb"));
    p_api.qemu_plugin_register_vcpu_tb_exec_cond_cb = reinterpret_cast<qemu_plugin_register_vcpu_tb_exec_cond_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_tb_exec_cond_cb"));
    p_api.qemu_plugin_register_vcpu_tb_exec_inline_per_vcpu = reinterpret_cast<
        qemu_plugin_register_vcpu_tb_exec_inline_per_vcpu_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_tb_exec_inline_per_vcpu"));
    p_api.qemu_plugin_register_vcpu_insn_exec_cb = reinterpret_cast<qemu_plugin_register_vcpu_insn_exec_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_insn_exec_cb"));
    p_api
        .qemu_plugin_register_vcpu_insn_exec_cond_cb = reinterpret_cast<qemu_plugin_register_vcpu_insn_exec_cond_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_insn_exec_cond_cb"));
    p_api.qemu_plugin_register_vcpu_insn_exec_inline_per_vcpu = reinterpret_cast<
        qemu_plugin_register_vcpu_insn_exec_inline_per_vcpu_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_insn_exec_inline_per_vcpu"));
    p_api.qemu_plugin_tb_n_insns = reinterpret_cast<qemu_plugin_tb_n_insns_fn>(
        m_lib->get_symbol("qemu_plugin_tb_n_insns"));
    p_api.qemu_plugin_tb_vaddr = reinterpret_cast<qemu_plugin_tb_vaddr_fn>(m_lib->get_symbol("qemu_plugin_tb_vaddr"));
    p_api.qemu_plugin_tb_get_insn = reinterpret_cast<qemu_plugin_tb_get_insn_fn>(
        m_lib->get_symbol("qemu_plugin_tb_get_insn"));
    p_api.qemu_plugin_insn_data = reinterpret_cast<qemu_plugin_insn_data_fn>(
        m_lib->get_symbol("qemu_plugin_insn_data"));
    p_api.qemu_plugin_insn_size = reinterpret_cast<qemu_plugin_insn_size_fn>(
        m_lib->get_symbol("qemu_plugin_insn_size"));
    p_api.qemu_plugin_insn_vaddr = reinterpret_cast<qemu_plugin_insn_vaddr_fn>(
        m_lib->get_symbol("qemu_plugin_insn_vaddr"));
    p_api.qemu_plugin_insn_haddr = reinterpret_cast<qemu_plugin_insn_haddr_fn>(
        m_lib->get_symbol("qemu_plugin_insn_haddr"));
    p_api.qemu_plugin_mem_size_shift = reinterpret_cast<qemu_plugin_mem_size_shift_fn>(
        m_lib->get_symbol("qemu_plugin_mem_size_shift"));
    p_api.qemu_plugin_mem_is_sign_extended = reinterpret_cast<qemu_plugin_mem_is_sign_extended_fn>(
        m_lib->get_symbol("qemu_plugin_mem_is_sign_extended"));
    p_api.qemu_plugin_mem_is_big_endian = reinterpret_cast<qemu_plugin_mem_is_big_endian_fn>(
        m_lib->get_symbol("qemu_plugin_mem_is_big_endian"));
    p_api.qemu_plugin_mem_is_store = reinterpret_cast<qemu_plugin_mem_is_store_fn>(
        m_lib->get_symbol("qemu_plugin_mem_is_store"));
    p_api.qemu_plugin_get_hwaddr = reinterpret_cast<qemu_plugin_get_hwaddr_fn>(
        m_lib->get_symbol("qemu_plugin_get_hwaddr"));
    p_api.qemu_plugin_hwaddr_is_io = reinterpret_cast<qemu_plugin_hwaddr_is_io_fn>(
        m_lib->get_symbol("qemu_plugin_hwaddr_is_io"));
    p_api.qemu_plugin_hwaddr_phys_addr = reinterpret_cast<qemu_plugin_hwaddr_phys_addr_fn>(
        m_lib->get_symbol("qemu_plugin_hwaddr_phys_addr"));
    p_api.qemu_plugin_hwaddr_device_name = reinterpret_cast<qemu_plugin_hwaddr_device_name_fn>(
        m_lib->get_symbol("qemu_plugin_hwaddr_device_name"));
    p_api.qemu_plugin_register_vcpu_mem_cb = reinterpret_cast<qemu_plugin_register_vcpu_mem_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_mem_cb"));
    p_api.qemu_plugin_register_vcpu_mem_inline_per_vcpu = reinterpret_cast<
        qemu_plugin_register_vcpu_mem_inline_per_vcpu_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_mem_inline_per_vcpu"));
    p_api.qemu_plugin_request_time_control = reinterpret_cast<qemu_plugin_request_time_control_fn>(
        m_lib->get_symbol("qemu_plugin_request_time_control"));
    p_api.qemu_plugin_update_ns = reinterpret_cast<qemu_plugin_update_ns_fn>(
        m_lib->get_symbol("qemu_plugin_update_ns"));
    p_api.qemu_plugin_register_vcpu_syscall_cb = reinterpret_cast<qemu_plugin_register_vcpu_syscall_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_syscall_cb"));
    p_api.qemu_plugin_register_vcpu_syscall_ret_cb = reinterpret_cast<qemu_plugin_register_vcpu_syscall_ret_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_vcpu_syscall_ret_cb"));
    p_api.qemu_plugin_insn_disas = reinterpret_cast<qemu_plugin_insn_disas_fn>(
        m_lib->get_symbol("qemu_plugin_insn_disas"));
    p_api.qemu_plugin_insn_symbol = reinterpret_cast<qemu_plugin_insn_symbol_fn>(
        m_lib->get_symbol("qemu_plugin_insn_symbol"));
    p_api.qemu_plugin_vcpu_for_each = reinterpret_cast<qemu_plugin_vcpu_for_each_fn>(
        m_lib->get_symbol("qemu_plugin_vcpu_for_each"));
    p_api.qemu_plugin_register_flush_cb = reinterpret_cast<qemu_plugin_register_flush_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_flush_cb"));
    p_api.qemu_plugin_register_atexit_cb = reinterpret_cast<qemu_plugin_register_atexit_cb_fn>(
        m_lib->get_symbol("qemu_plugin_register_atexit_cb"));
    p_api.qemu_plugin_num_vcpus = reinterpret_cast<qemu_plugin_num_vcpus_fn>(
        m_lib->get_symbol("qemu_plugin_num_vcpus"));
    p_api.qemu_plugin_outs = reinterpret_cast<qemu_plugin_outs_fn>(m_lib->get_symbol("qemu_plugin_outs"));
    p_api.qemu_plugin_bool_parse = reinterpret_cast<qemu_plugin_bool_parse_fn>(
        m_lib->get_symbol("qemu_plugin_bool_parse"));
    p_api.qemu_plugin_path_to_binary = reinterpret_cast<qemu_plugin_path_to_binary_fn>(
        m_lib->get_symbol("qemu_plugin_path_to_binary"));
    p_api.qemu_plugin_start_code = reinterpret_cast<qemu_plugin_start_code_fn>(
        m_lib->get_symbol("qemu_plugin_start_code"));
    p_api.qemu_plugin_end_code = reinterpret_cast<qemu_plugin_end_code_fn>(m_lib->get_symbol("qemu_plugin_end_code"));
    p_api.qemu_plugin_entry_code = reinterpret_cast<qemu_plugin_entry_code_fn>(
        m_lib->get_symbol("qemu_plugin_entry_code"));
    p_api.qemu_plugin_get_registers = reinterpret_cast<qemu_plugin_get_registers_fn>(
        m_lib->get_symbol("qemu_plugin_get_registers"));
    p_api.qemu_plugin_read_register = reinterpret_cast<qemu_plugin_read_register_fn>(
        m_lib->get_symbol("qemu_plugin_read_register"));
    p_api.qemu_plugin_scoreboard_new = reinterpret_cast<qemu_plugin_scoreboard_new_fn>(
        m_lib->get_symbol("qemu_plugin_scoreboard_new"));
    p_api.qemu_plugin_scoreboard_free = reinterpret_cast<qemu_plugin_scoreboard_free_fn>(
        m_lib->get_symbol("qemu_plugin_scoreboard_free"));
    p_api.qemu_plugin_scoreboard_find = reinterpret_cast<qemu_plugin_scoreboard_find_fn>(
        m_lib->get_symbol("qemu_plugin_scoreboard_find"));
    p_api.qemu_plugin_u64_add = reinterpret_cast<qemu_plugin_u64_add_fn>(m_lib->get_symbol("qemu_plugin_u64_add"));
    p_api.qemu_plugin_u64_get = reinterpret_cast<qemu_plugin_u64_get_fn>(m_lib->get_symbol("qemu_plugin_u64_get"));
    p_api.qemu_plugin_u64_set = reinterpret_cast<qemu_plugin_u64_set_fn>(m_lib->get_symbol("qemu_plugin_u64_set"));
    p_api.qemu_plugin_u64_sum = reinterpret_cast<qemu_plugin_u64_sum_fn>(m_lib->get_symbol("qemu_plugin_u64_sum"));

    exports = qemu_init(m_qemu_argv.size(), &m_qemu_argv[0]);

    m_int = std::make_shared<LibQemuInternals>(*this, exports);
    init_callbacks();
}

void LibQemu::start_gdb_server(std::string port)
{
    m_int->exports().gdbserver_start(port.c_str());
    m_int->exports().libqemu_set_autostart(m_auto_start);
}

void LibQemu::vm_start() { m_int->exports().vm_start(); }

void LibQemu::vm_stop_paused() { m_int->exports().vm_stop_paused(); }

int64_t LibQemu::get_virtual_clock() { return m_int->exports().clock_virtual_get_ns(); }

void LibQemu::tb_invalidate_phys_range(uint64_t start, uint64_t end)
{
    m_int->exports().tb_invalidate_phys_range(start, end);
}
QemuObject* LibQemu::object_new_unparented(const char* type_name) { return m_int->exports().object_new(type_name); }
QemuObject* LibQemu::object_new_internal(const char* type_name)
{
    QemuObject* o = object_new_unparented(type_name);
    QemuObject* root = m_int->exports().object_get_root();
    char name[20]; // helpful for debugging.
    snprintf(name, sizeof(name), "qbox-%.10s[*]", type_name);
    m_int->exports().object_property_add_child(root, name, o);
    return o;
}

Object LibQemu::object_new(const char* type_name)
{
    Object o(object_new_internal(type_name), m_int);
    return o;
}

std::shared_ptr<MemoryRegionOps> LibQemu::memory_region_ops_new()
{
    QemuMemoryRegionOps* ops = m_int->exports().mr_ops_new();

    return std::make_shared<MemoryRegionOps>(ops, m_int);
}

std::shared_ptr<AddressSpace> LibQemu::address_space_new()
{
    QemuAddressSpace* as = m_int->exports().address_space_new();

    return std::make_shared<AddressSpace>(as, m_int);
}

std::shared_ptr<AddressSpace> LibQemu::address_space_get_system_memory()
{
    QemuAddressSpace* as = m_int->exports().address_space_get_system_memory();
    return std::make_shared<AddressSpace>(as, m_int);
}

std::shared_ptr<MemoryRegion> LibQemu::get_system_memory()
{
    QemuMemoryRegion* mr = m_int->exports().get_system_memory();
    return std::make_shared<MemoryRegion>(mr, m_int);
}

std::shared_ptr<MemoryListener> LibQemu::memory_listener_new()
{
    auto ret = std::make_shared<MemoryListener>(m_int);
    QemuMemoryListener* ml = m_int->exports().memory_listener_new(ret.get(), "QboxMemoryListener");
    ret->set_ml(ml);
    return ret;
}

static void qemu_gpio_generic_handler(void* opaque, int n, int level)
{
    Gpio::GpioProxy* proxy = reinterpret_cast<Gpio::GpioProxy*>(opaque);

    proxy->event(level);
}

Gpio LibQemu::gpio_new()
{
    std::shared_ptr<Gpio::GpioProxy> proxy = std::make_shared<Gpio::GpioProxy>();

    QemuGpio* qemu_gpio = m_int->exports().gpio_new(qemu_gpio_generic_handler, proxy.get());

    Gpio gpio(Object(reinterpret_cast<QemuObject*>(qemu_gpio), m_int));
    gpio.set_proxy(proxy);

    return gpio;
}

std::shared_ptr<Timer> LibQemu::timer_new() { return std::make_shared<Timer>(m_int); }

Chardev LibQemu::chardev_new(const char* label, const char* type)
{
    QemuChardev* qemu_char_dev = m_int->exports().char_dev_new(label, type);
    Chardev ret(Object(reinterpret_cast<QemuObject*>(qemu_char_dev), m_int));

    return ret;
}

void LibQemu::check_cast(Object& o, const char* type) { /* TODO */ }

void LibQemu::lock_iothread() { m_int->exports().qemu_mutex_lock_iothread(); }

void LibQemu::unlock_iothread() { m_int->exports().qemu_mutex_unlock_iothread(); }

void LibQemu::rcu_read_lock() { m_int->exports().rcu_read_lock(); }

void LibQemu::rcu_read_unlock() { m_int->exports().rcu_read_unlock(); }

RcuReadLock LibQemu::rcu_read_lock_new() { return RcuReadLock(m_int); }

void LibQemu::coroutine_yield() { m_int->exports().coroutine_yield(); }

void LibQemu::finish_qemu_init() { m_int->exports().finish_qemu_init(); }

void LibQemu::system_reset() { m_int->exports().system_reset(); }

Bus LibQemu::sysbus_get_default()
{
    QemuBus* qemu_bus = m_int->exports().sysbus_get_default();
    Object obj(reinterpret_cast<QemuObject*>(qemu_bus), m_int);

    return Bus(obj);
}

void LibQemu::enable_opengl() { m_int->exports().enable_opengl(); }

DisplayOptions LibQemu::display_options_new()
{
    ::DisplayOptions* opts = m_int->exports().display_options_new();
    return DisplayOptions(opts, m_int);
}

Console LibQemu::console_lookup_by_index(int index)
{
    QemuConsole* qemu_cons = m_int->exports().console_lookup_by_index(index);
    return Console(qemu_cons, m_int);
}

std::vector<Console> LibQemu::get_all_consoles()
{
    std::vector<Console> consoles;
    int i = 0;
    while (QemuConsole* qemu_cons = m_int->exports().console_lookup_by_index(i)) {
        consoles.push_back(Console(qemu_cons, m_int));
        ++i;
    }
    return consoles;
}

int LibQemu::sdl2_init() const { return m_int->exports().sdl2_init(); }

const char* LibQemu::sdl2_get_error() const { return m_int->exports().sdl2_get_error(); }

std::vector<SDL2Console> LibQemu::sdl2_create_consoles(int num)
{
    m_int->exports().sdl2_create_consoles(num);
    std::vector<SDL2Console> ret;
    for (int i = 0; i < num; ++i) {
        sdl2_console* cons = m_int->exports().sdl2_get_console(i);
        ret.push_back(SDL2Console(cons, m_int));
    }
    return ret;
}

void LibQemu::sdl2_cleanup() { m_int->exports().sdl_cleanup(); }

DisplayGLCtxOps LibQemu::display_gl_ctx_ops_new(LibQemuIsCompatibleDclFn is_compatible_dcl_fn)
{
    ::DisplayGLCtxOps* ops = m_int->exports().display_gl_ctx_ops_new(is_compatible_dcl_fn);
    return DisplayGLCtxOps(ops, m_int);
}

Dcl LibQemu::dcl_new(DisplayChangeListener* dcl)
{
    assert(dcl != nullptr);
    return Dcl(dcl, m_int);
}

DclOps LibQemu::dcl_ops_new()
{
    DisplayChangeListenerOps* ops = m_int->exports().dcl_ops_new();
    return DclOps(ops, m_int);
}

void LibQemu::sdl2_2d_update(DisplayChangeListener* dcl, int x, int y, int w, int h)
{
    m_int->exports().sdl2_2d_update(dcl, x, y, w, h);
}

void LibQemu::sdl2_2d_switch(DisplayChangeListener* dcl, DisplaySurface* new_surface)
{
    m_int->exports().sdl2_2d_switch(dcl, new_surface);
}

void LibQemu::sdl2_2d_refresh(DisplayChangeListener* dcl) { m_int->exports().sdl2_2d_refresh(dcl); }

void LibQemu::sdl2_gl_update(DisplayChangeListener* dcl, int x, int y, int w, int h)
{
    m_int->exports().sdl2_gl_update(dcl, x, y, w, h);
}

void LibQemu::sdl2_gl_switch(DisplayChangeListener* dcl, DisplaySurface* new_surface)
{
    m_int->exports().sdl2_gl_switch(dcl, new_surface);
}

void LibQemu::dcl_dpy_gfx_replace_surface(DisplayChangeListener* dcl, DisplaySurface* new_surface)
{
    m_int->exports().dcl_dpy_gfx_replace_surface(dcl, new_surface);
}

void LibQemu::sdl2_gl_refresh(DisplayChangeListener* dcl) { m_int->exports().sdl2_gl_refresh(dcl); }

void LibQemu::sdl2_window_create(DisplayChangeListener* dcl) { m_int->exports().sdl2_window_create(dcl); }

void LibQemu::sdl2_window_destroy(DisplayChangeListener* dcl) { m_int->exports().sdl2_window_destroy(dcl); }

void LibQemu::sdl2_window_resize(DisplayChangeListener* dcl) { m_int->exports().sdl2_window_resize(dcl); }

void LibQemu::sdl2_poll_events(DisplayChangeListener* dcl) { m_int->exports().sdl2_poll_events(dcl); }

QEMUGLContext LibQemu::sdl2_gl_create_context(DisplayGLCtx* dgc, QEMUGLParams* p)
{
    return m_int->exports().sdl2_gl_create_context(dgc, p);
}

void LibQemu::sdl2_gl_destroy_context(DisplayGLCtx* dgc, QEMUGLContext gl_ctx)
{
    m_int->exports().sdl2_gl_destroy_context(dgc, gl_ctx);
}

int LibQemu::sdl2_gl_make_context_current(DisplayGLCtx* dgc, QEMUGLContext gl_ctx)
{
    return m_int->exports().sdl2_gl_make_context_current(dgc, gl_ctx);
}

bool LibQemu::virgl_has_blob() const { return m_int->exports().virgl_has_blob(); }

} // namespace qemu
