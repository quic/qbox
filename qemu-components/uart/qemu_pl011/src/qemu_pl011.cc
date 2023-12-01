#include "qemu_pl011.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(qemu_pl011, sc_core::sc_object*);
}