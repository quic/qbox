#include "libqbox/components/irq-ctrl/arm_gicv3.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(arm_gicv3, sc_core::sc_object*);
}