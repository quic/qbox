#include "libqbox/qemu-instance.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(QemuInstanceManager);
    GSC_MODULE_REGISTER_C(QemuInstance, sc_core::sc_object*, std::string);
}
