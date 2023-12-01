#include <qemu_xhci.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(qemu_xhci, sc_core::sc_object*, sc_core::sc_object*);
}