#include <plic-sifive.h>

void module_register() { GSC_MODULE_REGISTER_C(plic_sifive, sc_core::sc_object*); }