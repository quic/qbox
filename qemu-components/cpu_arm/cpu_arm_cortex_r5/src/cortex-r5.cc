#include <cortex-r5.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(cpu_arm_cortexR5, sc_core::sc_object*);
}