#include "libqbox-extra/components/pci/rtl8139_pci.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(rtl8139_pci, sc_core::sc_object*, sc_core::sc_object*);
}