#include <cortex-a76.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(cpu_arm_cortexA76, sc_core::sc_object*);
}