#include <cortex-a55.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(cpu_arm_cortexA55, sc_core::sc_object*);
}