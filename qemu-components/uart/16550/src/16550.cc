#include <16550.h>

void module_register() { GSC_MODULE_REGISTER_C(uart_16550, sc_core::sc_object*); }