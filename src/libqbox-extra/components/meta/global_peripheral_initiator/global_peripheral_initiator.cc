#include "libqbox-extra/components/meta/global_peripheral_initiator.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(global_peripheral_initiator, sc_core::sc_object*, sc_core::sc_object*);
}