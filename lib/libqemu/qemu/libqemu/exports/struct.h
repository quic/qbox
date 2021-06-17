#ifndef _LIBQEMU_EXPORTS_H
#define _LIBQEMU_EXPORTS_H

/* auto-generated - do not modify */

#include "libqemu/exports/typedefs.h"
typedef struct LibQemuExports LibQemuExports;

struct LibQemuExports {
    qemu_mutex_lock_iothread_fn qemu_mutex_lock_iothread;
    qemu_mutex_unlock_iothread_fn qemu_mutex_unlock_iothread;
    object_new_fn object_new;
    object_ref_fn object_ref;
    object_unref_fn object_unref;
    object_property_set_str_fn object_property_set_str;
    object_property_set_link_fn object_property_set_link;
    object_property_set_bool_fn object_property_set_bool;
    object_property_set_int_fn object_property_set_int;
    object_property_set_uint_fn object_property_set_uint;
    object_property_add_child_fn object_property_add_child;
    object_get_root_fn object_get_root;
    memory_region_new_fn memory_region_new;
    memory_region_init_io_fn memory_region_init_io;
    memory_region_init_ram_ptr_fn memory_region_init_ram_ptr;
    memory_region_init_alias_fn memory_region_init_alias;
    memory_region_add_subregion_fn memory_region_add_subregion;
    memory_region_del_subregion_fn memory_region_del_subregion;
    memory_region_dispatch_read_fn memory_region_dispatch_read;
    memory_region_dispatch_write_fn memory_region_dispatch_write;
    memory_region_size_fn memory_region_size;
    mr_ops_new_fn mr_ops_new;
    mr_ops_free_fn mr_ops_free;
    mr_ops_set_read_cb_fn mr_ops_set_read_cb;
    mr_ops_set_write_cb_fn mr_ops_set_write_cb;
    mr_ops_set_max_access_size_fn mr_ops_set_max_access_size;
    gpio_new_fn gpio_new;
    gpio_set_fn gpio_set;
    qdev_get_child_bus_fn qdev_get_child_bus;
    qdev_set_parent_bus_fn qdev_set_parent_bus;
    qdev_connect_gpio_out_fn qdev_connect_gpio_out;
    qdev_connect_gpio_out_named_fn qdev_connect_gpio_out_named;
    qdev_get_gpio_in_fn qdev_get_gpio_in;
    qdev_get_gpio_in_named_fn qdev_get_gpio_in_named;
    sysbus_mmio_get_region_fn sysbus_mmio_get_region;
    sysbus_connect_gpio_out_fn sysbus_connect_gpio_out;
    cpu_loop_fn cpu_loop;
    cpu_loop_is_busy_fn cpu_loop_is_busy;
    cpu_can_run_fn cpu_can_run;
    cpu_register_thread_fn cpu_register_thread;
    cpu_kick_fn cpu_kick;
    cpu_reset_fn cpu_reset;
    cpu_halt_fn cpu_halt;
    current_cpu_get_fn current_cpu_get;
    current_cpu_set_fn current_cpu_set;
    async_safe_run_on_cpu_fn async_safe_run_on_cpu;
    gdbserver_start_fn gdbserver_start;
    clock_virtual_get_ns_fn clock_virtual_get_ns;
    timer_new_virtual_ns_fn timer_new_virtual_ns;
    timer_free_fn timer_free;
    timer_mod_ns_fn timer_mod_ns;
    timer_del_fn timer_del;
    cpu_arm_set_cp15_cbar_fn cpu_arm_set_cp15_cbar;
    cpu_arm_add_nvic_link_fn cpu_arm_add_nvic_link;
    arm_nvic_add_cpu_link_fn arm_nvic_add_cpu_link;
    cpu_aarch64_set_aarch64_mode_fn cpu_aarch64_set_aarch64_mode;
    cpu_arm_get_exclusive_val_fn cpu_arm_get_exclusive_val;
    tb_invalidate_phys_range_fn tb_invalidate_phys_range;
};

#endif
