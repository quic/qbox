#include "libqbox/components/cpu/arm/neoverse-n1.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(qemu_cpu_arm_neoverseN1, sc_core::sc_object*);
}