#include <riscv-aclint-mtimer.h>

void module_register() { GSC_MODULE_REGISTER_C(riscv_aclint_mtimer, sc_core::sc_object*); }