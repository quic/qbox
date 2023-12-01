#include <cortex-a53.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(cpu_arm_cortexA53, sc_core::sc_object*);
}