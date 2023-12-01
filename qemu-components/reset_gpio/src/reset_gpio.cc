#include <reset_gpio.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(reset_gpio, sc_core::sc_object*);
}