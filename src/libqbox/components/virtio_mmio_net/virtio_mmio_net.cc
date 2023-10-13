#include "libqbox/components/net/virtio_mmio_net.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(virtio_mmio_net, sc_core::sc_object*);
}