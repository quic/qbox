#include "nvme.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(nvme_disk, sc_core::sc_object*);
}