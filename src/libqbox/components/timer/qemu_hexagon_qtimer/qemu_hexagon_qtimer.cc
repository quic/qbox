#include "libqbox/components/timer/qemu_hexagon_qtimer.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(qemu_hexagon_qtimer, sc_core::sc_object*);
}