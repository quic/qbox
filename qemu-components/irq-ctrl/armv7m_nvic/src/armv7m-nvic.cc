#include <armv7m-nvic.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(nvic_armv7m, sc_core::sc_object*);
}