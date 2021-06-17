/* auto-generated - do not modify */

#include "qemu/osdep.h"


#include "qemu/main-loop.h"
#include "libqemu/exports/iothread-wrapper.h"
#include "libqemu/run-on-iothread.h"

Object * object_new_wrapper(const char * p0)
{
    bool was_locked = qemu_mutex_iothread_locked();
    if (!was_locked) {
        qemu_mutex_lock_iothread();
    }

    Object * ret = object_new(p0);

    if (!was_locked) {
        qemu_mutex_unlock_iothread();
    }

    return ret;
}
void object_unref_wrapper(Object * p0)
{
    bool was_locked = qemu_mutex_iothread_locked();
    if (!was_locked) {
        qemu_mutex_lock_iothread();
    }

    object_unref(p0);

    if (!was_locked) {
        qemu_mutex_unlock_iothread();
    }

}
struct object_property_set_str_params {
    Object * p0;
    const char * p1;
    const char * p2;
    Error ** p3;
};

static void object_property_set_str_worker(void *opaque)
{
    struct object_property_set_str_params *params = (struct object_property_set_str_params *) opaque;
    object_property_set_str(params->p0, params->p1, params->p2, params->p3);
}

void object_property_set_str_wrapper(Object * p0, const char * p1, const char * p2, Error ** p3)
{
    struct object_property_set_str_params params;

    params.p0 = p0;
    params.p1 = p1;
    params.p2 = p2;
    params.p3 = p3;

    run_on_iothread(object_property_set_str_worker, &params);
}

struct object_property_set_link_params {
    Object * p0;
    const char * p1;
    Object * p2;
    Error ** p3;
};

static void object_property_set_link_worker(void *opaque)
{
    struct object_property_set_link_params *params = (struct object_property_set_link_params *) opaque;
    object_property_set_link(params->p0, params->p1, params->p2, params->p3);
}

void object_property_set_link_wrapper(Object * p0, const char * p1, Object * p2, Error ** p3)
{
    struct object_property_set_link_params params;

    params.p0 = p0;
    params.p1 = p1;
    params.p2 = p2;
    params.p3 = p3;

    run_on_iothread(object_property_set_link_worker, &params);
}

struct object_property_set_bool_params {
    Object * p0;
    const char * p1;
    bool p2;
    Error ** p3;
};

static void object_property_set_bool_worker(void *opaque)
{
    struct object_property_set_bool_params *params = (struct object_property_set_bool_params *) opaque;
    object_property_set_bool(params->p0, params->p1, params->p2, params->p3);
}

void object_property_set_bool_wrapper(Object * p0, const char * p1, bool p2, Error ** p3)
{
    struct object_property_set_bool_params params;

    params.p0 = p0;
    params.p1 = p1;
    params.p2 = p2;
    params.p3 = p3;

    run_on_iothread(object_property_set_bool_worker, &params);
}

struct object_property_set_int_params {
    Object * p0;
    const char * p1;
    int64_t p2;
    Error ** p3;
};

static void object_property_set_int_worker(void *opaque)
{
    struct object_property_set_int_params *params = (struct object_property_set_int_params *) opaque;
    object_property_set_int(params->p0, params->p1, params->p2, params->p3);
}

void object_property_set_int_wrapper(Object * p0, const char * p1, int64_t p2, Error ** p3)
{
    struct object_property_set_int_params params;

    params.p0 = p0;
    params.p1 = p1;
    params.p2 = p2;
    params.p3 = p3;

    run_on_iothread(object_property_set_int_worker, &params);
}

struct object_property_set_uint_params {
    Object * p0;
    const char * p1;
    uint64_t p2;
    Error ** p3;
};

static void object_property_set_uint_worker(void *opaque)
{
    struct object_property_set_uint_params *params = (struct object_property_set_uint_params *) opaque;
    object_property_set_uint(params->p0, params->p1, params->p2, params->p3);
}

void object_property_set_uint_wrapper(Object * p0, const char * p1, uint64_t p2, Error ** p3)
{
    struct object_property_set_uint_params params;

    params.p0 = p0;
    params.p1 = p1;
    params.p2 = p2;
    params.p3 = p3;

    run_on_iothread(object_property_set_uint_worker, &params);
}

void object_property_add_child_wrapper(Object * p0, const char * p1, Object * p2)
{
    bool was_locked = qemu_mutex_iothread_locked();
    if (!was_locked) {
        qemu_mutex_lock_iothread();
    }

    object_property_add_child(p0, p1, p2);

    if (!was_locked) {
        qemu_mutex_unlock_iothread();
    }

}
void memory_region_add_subregion_wrapper(MemoryRegion * p0, hwaddr p1, MemoryRegion * p2)
{
    bool was_locked = qemu_mutex_iothread_locked();
    if (!was_locked) {
        qemu_mutex_lock_iothread();
    }

    memory_region_add_subregion(p0, p1, p2);

    if (!was_locked) {
        qemu_mutex_unlock_iothread();
    }

}
void memory_region_del_subregion_wrapper(MemoryRegion * p0, MemoryRegion * p1)
{
    bool was_locked = qemu_mutex_iothread_locked();
    if (!was_locked) {
        qemu_mutex_lock_iothread();
    }

    memory_region_del_subregion(p0, p1);

    if (!was_locked) {
        qemu_mutex_unlock_iothread();
    }

}
MemTxResult memory_region_dispatch_read_wrapper(MemoryRegion * p0, hwaddr p1, uint64_t * p2, unsigned int p3, MemTxAttrs p4)
{
    bool was_locked = qemu_mutex_iothread_locked();
    if (!was_locked) {
        qemu_mutex_lock_iothread();
    }

    MemTxResult ret = memory_region_dispatch_read(p0, p1, p2, p3, p4);

    if (!was_locked) {
        qemu_mutex_unlock_iothread();
    }

    return ret;
}
MemTxResult memory_region_dispatch_write_wrapper(MemoryRegion * p0, hwaddr p1, uint64_t p2, unsigned int p3, MemTxAttrs p4)
{
    bool was_locked = qemu_mutex_iothread_locked();
    if (!was_locked) {
        qemu_mutex_lock_iothread();
    }

    MemTxResult ret = memory_region_dispatch_write(p0, p1, p2, p3, p4);

    if (!was_locked) {
        qemu_mutex_unlock_iothread();
    }

    return ret;
}
void qemu_set_irq_wrapper(struct IRQState * p0, int p1)
{
    bool was_locked = qemu_mutex_iothread_locked();
    if (!was_locked) {
        qemu_mutex_lock_iothread();
    }

    qemu_set_irq(p0, p1);

    if (!was_locked) {
        qemu_mutex_unlock_iothread();
    }

}
struct gdbserver_start_params {
    const char * p0;
};

static void gdbserver_start_worker(void *opaque)
{
    struct gdbserver_start_params *params = (struct gdbserver_start_params *) opaque;
    gdbserver_start(params->p0);
}

void gdbserver_start_wrapper(const char * p0)
{
    struct gdbserver_start_params params;

    params.p0 = p0;

    run_on_iothread(gdbserver_start_worker, &params);
}

