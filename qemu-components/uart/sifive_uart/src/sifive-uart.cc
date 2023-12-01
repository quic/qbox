#include "sifive-uart.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(sifive_uart, sc_core::sc_object*);
}