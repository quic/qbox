#include "libqbox-extra/components/pci/virtio_gpu_pci.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(virtio_gpu_pci, sc_core::sc_object*, sc_core::sc_object*);
}
