#ifndef _LIBQEMU_IOTHREAD_WRAPPER_H
#define _LIBQEMU_IOTHREAD_WRAPPER_H

/* auto-generated - do not modify */

#include "libqemu/wrappers/iothread.h"
#include "qemu/main-loop.h"
#include "qom/object.h"
#include "exec/memory.h"
#include "hw/irq.h"
#include "hw/qdev-core.h"
#include "hw/sysbus.h"
#include "libqemu/wrappers/cpu.h"
#include "exec/gdbstub.h"
#ifdef TARGET_AARCH64
#include "libqemu/wrappers/target/arm.h"
#endif
#include "exec/ram_addr.h"
#include "libqemu/wrappers/memory.h"
#include "exec/hwaddr.h"
#include "exec/memattrs.h"
#include "libqemu/wrappers/gpio.h"
#include "libqemu/wrappers/timer.h"
Object * object_new_wrapper(const char * p0);
void object_unref_wrapper(Object * p0);
void object_property_set_str_wrapper(Object * p0, const char * p1, const char * p2, Error ** p3);
void object_property_set_link_wrapper(Object * p0, const char * p1, Object * p2, Error ** p3);
void object_property_set_bool_wrapper(Object * p0, const char * p1, bool p2, Error ** p3);
void object_property_set_int_wrapper(Object * p0, const char * p1, int64_t p2, Error ** p3);
void object_property_set_uint_wrapper(Object * p0, const char * p1, uint64_t p2, Error ** p3);
void object_property_add_child_wrapper(Object * p0, const char * p1, Object * p2);
void memory_region_add_subregion_wrapper(MemoryRegion * p0, hwaddr p1, MemoryRegion * p2);
void memory_region_del_subregion_wrapper(MemoryRegion * p0, MemoryRegion * p1);
MemTxResult memory_region_dispatch_read_wrapper(MemoryRegion * p0, hwaddr p1, uint64_t * p2, unsigned int p3, MemTxAttrs p4);
MemTxResult memory_region_dispatch_write_wrapper(MemoryRegion * p0, hwaddr p1, uint64_t p2, unsigned int p3, MemTxAttrs p4);
void qemu_set_irq_wrapper(struct IRQState * p0, int p1);
void gdbserver_start_wrapper(const char * p0);

#endif
