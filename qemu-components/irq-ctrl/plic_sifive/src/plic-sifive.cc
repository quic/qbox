#include <plic-sifive.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(riscv_sifive_plic, sc_core::sc_object*);
}