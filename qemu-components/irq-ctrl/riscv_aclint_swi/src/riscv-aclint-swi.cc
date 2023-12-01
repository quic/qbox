#include <riscv-aclint-swi.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(riscv_aclint_swi, sc_core::sc_object*, sc_core::sc_object*);
}