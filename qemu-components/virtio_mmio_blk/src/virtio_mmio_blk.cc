#include <virtio_mmio_blk.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(virtio_mmio_blk, sc_core::sc_object*);
}
