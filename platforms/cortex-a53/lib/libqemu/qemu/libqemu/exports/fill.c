/* auto-generated - do not modify */

#include "qemu/osdep.h"

#include "exec/gdbstub.h"
#include "exec/memory.h"
#include "hw/irq.h"
#include "hw/qdev-core.h"
#include "hw/sysbus.h"
#include "libqemu/exports/iothread-wrapper.h"
#include "libqemu/fill.h"
#include "libqemu/wrappers/cpu.h"
#include "libqemu/wrappers/iothread.h"
#include "qemu/main-loop.h"
#include "qom/object.h"
#ifdef TARGET_AARCH64
#include "libqemu/wrappers/target/arm.h"
#endif
#include "exec/ram_addr.h"

void libqemu_exports_fill(LibQemuExports *exports) {
  exports->qemu_mutex_lock_iothread =
      (void (*)(void))libqemu_mutex_lock_iothread;
  exports->qemu_mutex_unlock_iothread =
      (void (*)(void))qemu_mutex_unlock_iothread;
  exports->object_new = (QemuObject * (*)(const char *)) object_new_wrapper;
  exports->object_ref = (void (*)(QemuObject *))object_ref;
  exports->object_unref = (void (*)(QemuObject *))object_unref_wrapper;
  exports->object_property_set_str =
      (void (*)(QemuObject *, const char *, const char *,
                QemuError **))object_property_set_str_wrapper;
  exports->object_property_set_link =
      (void (*)(QemuObject *, const char *, QemuObject *,
                QemuError **))object_property_set_link_wrapper;
  exports->object_property_set_bool =
      (void (*)(QemuObject *, const char *, bool,
                QemuError **))object_property_set_bool_wrapper;
  exports->object_property_set_int =
      (void (*)(QemuObject *, const char *, int64_t,
                QemuError **))object_property_set_int_wrapper;
  exports->object_property_set_uint =
      (void (*)(QemuObject *, const char *, uint64_t,
                QemuError **))object_property_set_uint_wrapper;
  exports->object_property_add_child =
      (void (*)(QemuObject *, const char *,
                QemuObject *))object_property_add_child_wrapper;
  exports->object_get_root = (QemuObject * (*)(void)) object_get_root;
  exports->memory_region_new =
      (QemuMemoryRegion * (*)(void)) libqemu_memory_region_new;
  exports->memory_region_init_io =
      (void (*)(QemuMemoryRegion *, QemuObject *, const QemuMemoryRegionOps *,
                void *, const char *, uint64_t))libqemu_memory_region_init_io;
  exports->memory_region_init_ram_ptr =
      (void (*)(QemuMemoryRegion *, QemuObject *, const char *, uint64_t,
                void *))memory_region_init_ram_ptr;
  exports->memory_region_init_alias =
      (void (*)(QemuMemoryRegion *, QemuObject *, const char *,
                QemuMemoryRegion *, hwaddr, uint64_t))memory_region_init_alias;
  exports->memory_region_add_subregion =
      (void (*)(QemuMemoryRegion *, hwaddr,
                QemuMemoryRegion *))memory_region_add_subregion_wrapper;
  exports->memory_region_del_subregion =
      (void (*)(QemuMemoryRegion *,
                QemuMemoryRegion *))memory_region_del_subregion_wrapper;
  exports->memory_region_dispatch_read =
      (MemTxResult(*)(QemuMemoryRegion *, hwaddr, uint64_t *, unsigned int,
                      MemTxAttrs))memory_region_dispatch_read_wrapper;
  exports->memory_region_dispatch_write =
      (MemTxResult(*)(QemuMemoryRegion *, hwaddr, uint64_t, unsigned int,
                      MemTxAttrs))memory_region_dispatch_write_wrapper;
  exports->memory_region_size =
      (uint64_t(*)(QemuMemoryRegion *))memory_region_size;
  exports->mr_ops_new = (QemuMemoryRegionOps * (*)(void)) libqemu_mr_ops_new;
  exports->mr_ops_free = (void (*)(QemuMemoryRegionOps *))libqemu_mr_ops_free;
  exports->mr_ops_set_read_cb = (void (*)(
      QemuMemoryRegionOps *, LibQemuMrReadCb))libqemu_mr_ops_set_read_cb;
  exports->mr_ops_set_write_cb = (void (*)(
      QemuMemoryRegionOps *, LibQemuMrWriteCb))libqemu_mr_ops_set_write_cb;
  exports->mr_ops_set_max_access_size =
      (void (*)(QemuMemoryRegionOps *, unsigned))libqemu_mr_ops_max_access_size;
  exports->gpio_new =
      (QemuGpio * (*)(LibQemuGpioHandlerFn, void *)) libqemu_gpio_new;
  exports->gpio_set = (void (*)(QemuGpio *, int))qemu_set_irq_wrapper;
  exports->qdev_get_child_bus =
      (QemuBus * (*)(QemuDevice *, const char *)) qdev_get_child_bus;
  exports->qdev_set_parent_bus =
      (void (*)(QemuDevice *, QemuBus *))qdev_set_parent_bus;
  exports->qdev_connect_gpio_out =
      (void (*)(QemuDevice *, int, QemuGpio *))qdev_connect_gpio_out;
  exports->qdev_connect_gpio_out_named = (void (*)(
      QemuDevice *, const char *, int, QemuGpio *))qdev_connect_gpio_out_named;
  exports->qdev_get_gpio_in =
      (QemuGpio * (*)(QemuDevice *, int)) qdev_get_gpio_in;
  exports->qdev_get_gpio_in_named =
      (QemuGpio * (*)(QemuDevice *, const char *, int)) qdev_get_gpio_in_named;
  exports->sysbus_mmio_get_region =
      (QemuMemoryRegion * (*)(QemuSysBusDevice *, int)) sysbus_mmio_get_region;
  exports->sysbus_connect_gpio_out =
      (void (*)(QemuSysBusDevice *, int, QemuGpio *))sysbus_connect_irq;
  exports->cpu_loop = (void (*)(QemuObject *))libqemu_cpu_loop;
  exports->cpu_loop_is_busy = (bool (*)(QemuObject *))libqemu_cpu_loop_is_busy;
  exports->cpu_can_run = (bool (*)(QemuObject *))libqemu_cpu_can_run;
  exports->cpu_register_thread =
      (void (*)(QemuObject *))libqemu_cpu_register_thread;
  exports->cpu_kick = (void (*)(QemuObject *))qemu_cpu_kick;
  exports->cpu_reset = (void (*)(QemuObject *))libqemu_cpu_reset;
  exports->cpu_halt = (void (*)(QemuObject *, bool))libqemu_cpu_halt;
  exports->current_cpu_get = (QemuObject * (*)(void)) libqemu_current_cpu_get;
  exports->current_cpu_set = (void (*)(QemuObject *))libqemu_current_cpu_set;
  exports->async_safe_run_on_cpu = (void (*)(
      QemuObject *, void (*)(void *), void *))libqemu_async_safe_run_on_cpu;
  exports->gdbserver_start = (void (*)(const char *))gdbserver_start_wrapper;
  exports->clock_virtual_get_ns =
      (int64_t(*)(void))libqemu_clock_virtual_get_ns;
  exports->timer_new_virtual_ns =
      (QemuTimer * (*)(LibQemuTimerCb, void *)) libqemu_timer_new_virtual_ns;
  exports->timer_free = (void (*)(QemuTimer *))timer_free;
  exports->timer_mod_ns = (void (*)(QemuTimer *, int64_t))timer_mod_ns;
  exports->timer_del = (void (*)(QemuTimer *))timer_del;
#ifdef TARGET_AARCH64
  exports->cpu_arm_set_cp15_cbar =
      (void (*)(QemuObject *, uint64_t))libqemu_cpu_arm_set_cp15_cbar;
#else
  exports->cpu_arm_set_cp15_cbar = NULL;
#endif
#ifdef TARGET_AARCH64
  exports->cpu_arm_add_nvic_link =
      (void (*)(QemuObject *))libqemu_cpu_arm_add_nvic_link;
#else
  exports->cpu_arm_add_nvic_link = NULL;
#endif
#ifdef TARGET_AARCH64
  exports->arm_nvic_add_cpu_link =
      (void (*)(QemuObject *))libqemu_arm_nvic_add_cpu_link;
#else
  exports->arm_nvic_add_cpu_link = NULL;
#endif
#ifdef TARGET_AARCH64
  exports->cpu_aarch64_set_aarch64_mode =
      (void (*)(QemuObject *, bool))libqemu_cpu_aarch64_set_aarch64_mode;
#else
  exports->cpu_aarch64_set_aarch64_mode = NULL;
#endif
#ifdef TARGET_AARCH64
  exports->cpu_arm_get_exclusive_val =
      (uint64_t(*)(QemuObject *))libqemu_cpu_arm_get_exclusive_val;
#else
  exports->cpu_arm_get_exclusive_val = NULL;
#endif
  exports->tb_invalidate_phys_range =
      (void (*)(uint64_t, uint64_t))tb_invalidate_phys_range;
}
