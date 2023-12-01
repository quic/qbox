#include <cortex-m55.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(cpu_arm_cortexM55, sc_core::sc_object*);
}