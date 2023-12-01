#include <hexagon.h>

void module_register()
{
    GSC_MODULE_REGISTER_C(qemu_cpu_hexagon, sc_core::sc_object*, uint32_t, uint32_t, uint32_t, uint32_t, bool);
}