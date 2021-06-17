#ifndef _LIBQEMU_TYPEDEFS_H
#define _LIBQEMU_TYPEDEFS_H

/* auto-generated - do not modify */

#include "libqemu/wrappers/memory.h"
#include "exec/hwaddr.h"
#include "exec/memattrs.h"
#include "libqemu/wrappers/gpio.h"
#include "libqemu/wrappers/timer.h"

typedef struct QemuError QemuError;
typedef struct QemuObject QemuObject;
typedef struct QemuMemoryRegion QemuMemoryRegion;
typedef struct QemuMemoryRegionOps QemuMemoryRegionOps;
typedef struct QemuGpio QemuGpio;
typedef struct QemuDevice QemuDevice;
typedef struct QemuBus QemuBus;
typedef struct QemuSysBusDevice QemuSysBusDevice;
typedef struct QemuTimer QemuTimer;
typedef void (*qemu_mutex_lock_iothread_fn)(void);
typedef void (*qemu_mutex_unlock_iothread_fn)(void);
typedef QemuObject * (*object_new_fn)(const char *);
typedef void (*object_ref_fn)(QemuObject *);
typedef void (*object_unref_fn)(QemuObject *);
typedef void (*object_property_set_str_fn)(QemuObject *, const char *, const char *, QemuError **);
typedef void (*object_property_set_link_fn)(QemuObject *, const char *, QemuObject *, QemuError **);
typedef void (*object_property_set_bool_fn)(QemuObject *, const char *, bool, QemuError **);
typedef void (*object_property_set_int_fn)(QemuObject *, const char *, int64_t, QemuError **);
typedef void (*object_property_set_uint_fn)(QemuObject *, const char *, uint64_t, QemuError **);
typedef void (*object_property_add_child_fn)(QemuObject *, const char *, QemuObject *);
typedef QemuObject * (*object_get_root_fn)(void);
typedef QemuMemoryRegion * (*memory_region_new_fn)(void);
typedef void (*memory_region_init_io_fn)(QemuMemoryRegion *, QemuObject *, const QemuMemoryRegionOps *, void *, const char *, uint64_t);
typedef void (*memory_region_init_ram_ptr_fn)(QemuMemoryRegion *, QemuObject *, const char *, uint64_t, void *);
typedef void (*memory_region_init_alias_fn)(QemuMemoryRegion *, QemuObject *, const char *, QemuMemoryRegion *, hwaddr, uint64_t);
typedef void (*memory_region_add_subregion_fn)(QemuMemoryRegion *, hwaddr, QemuMemoryRegion *);
typedef void (*memory_region_del_subregion_fn)(QemuMemoryRegion *, QemuMemoryRegion *);
typedef MemTxResult (*memory_region_dispatch_read_fn)(QemuMemoryRegion *, hwaddr, uint64_t *, unsigned int, MemTxAttrs);
typedef MemTxResult (*memory_region_dispatch_write_fn)(QemuMemoryRegion *, hwaddr, uint64_t, unsigned int, MemTxAttrs);
typedef uint64_t (*memory_region_size_fn)(QemuMemoryRegion *);
typedef QemuMemoryRegionOps * (*mr_ops_new_fn)(void);
typedef void (*mr_ops_free_fn)(QemuMemoryRegionOps *);
typedef void (*mr_ops_set_read_cb_fn)(QemuMemoryRegionOps *, LibQemuMrReadCb);
typedef void (*mr_ops_set_write_cb_fn)(QemuMemoryRegionOps *, LibQemuMrWriteCb);
typedef void (*mr_ops_set_max_access_size_fn)(QemuMemoryRegionOps *, unsigned);
typedef QemuGpio * (*gpio_new_fn)(LibQemuGpioHandlerFn, void *);
typedef void (*gpio_set_fn)(QemuGpio *, int);
typedef QemuBus * (*qdev_get_child_bus_fn)(QemuDevice *, const char *);
typedef void (*qdev_set_parent_bus_fn)(QemuDevice *, QemuBus *);
typedef void (*qdev_connect_gpio_out_fn)(QemuDevice *, int, QemuGpio *);
typedef void (*qdev_connect_gpio_out_named_fn)(QemuDevice *, const char *, int, QemuGpio *);
typedef QemuGpio * (*qdev_get_gpio_in_fn)(QemuDevice *, int);
typedef QemuGpio * (*qdev_get_gpio_in_named_fn)(QemuDevice *, const char *, int);
typedef QemuMemoryRegion * (*sysbus_mmio_get_region_fn)(QemuSysBusDevice *, int);
typedef void (*sysbus_connect_gpio_out_fn)(QemuSysBusDevice *, int, QemuGpio *);
typedef void (*cpu_loop_fn)(QemuObject *);
typedef bool (*cpu_loop_is_busy_fn)(QemuObject *);
typedef bool (*cpu_can_run_fn)(QemuObject *);
typedef void (*cpu_register_thread_fn)(QemuObject *);
typedef void (*cpu_kick_fn)(QemuObject *);
typedef void (*cpu_reset_fn)(QemuObject *);
typedef void (*cpu_halt_fn)(QemuObject *, bool);
typedef QemuObject * (*current_cpu_get_fn)(void);
typedef void (*current_cpu_set_fn)(QemuObject *);
typedef void (*async_safe_run_on_cpu_fn)(QemuObject *, void (*)(void *), void *);
typedef void (*gdbserver_start_fn)(const char *);
typedef int64_t (*clock_virtual_get_ns_fn)(void);
typedef QemuTimer * (*timer_new_virtual_ns_fn)(LibQemuTimerCb, void *);
typedef void (*timer_free_fn)(QemuTimer *);
typedef void (*timer_mod_ns_fn)(QemuTimer *, int64_t);
typedef void (*timer_del_fn)(QemuTimer *);
typedef void (*cpu_arm_set_cp15_cbar_fn)(QemuObject *, uint64_t);
typedef void (*cpu_arm_add_nvic_link_fn)(QemuObject *);
typedef void (*arm_nvic_add_cpu_link_fn)(QemuObject *);
typedef void (*cpu_aarch64_set_aarch64_mode_fn)(QemuObject *, bool);
typedef uint64_t (*cpu_arm_get_exclusive_val_fn)(QemuObject *);
typedef void (*tb_invalidate_phys_range_fn)(uint64_t, uint64_t);

#endif
