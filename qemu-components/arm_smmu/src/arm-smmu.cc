#include <arm-smmu.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(arm_smmu, sc_core::sc_object*);
}