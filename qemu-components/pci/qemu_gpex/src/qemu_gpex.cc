#include "qemu_gpex.h"

void module_register() { GSC_MODULE_REGISTER_C(qemu_gpex, sc_core::sc_object*); }