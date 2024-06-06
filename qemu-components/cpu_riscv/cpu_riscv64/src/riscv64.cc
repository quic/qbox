#include <riscv64.h>

void module_register() { GSC_MODULE_REGISTER_C(cpu_riscv64, sc_core::sc_object*, uint64_t); }