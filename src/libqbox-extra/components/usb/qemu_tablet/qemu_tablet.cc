#include "libqbox-extra/components/usb/qemu_tablet.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(qemu_tablet, sc_core::sc_object*, sc_core::sc_object*);
}