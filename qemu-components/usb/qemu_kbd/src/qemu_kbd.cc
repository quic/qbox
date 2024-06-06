#include <qemu_kbd.h>

void module_register() { GSC_MODULE_REGISTER_C(qemu_kbd, sc_core::sc_object*, sc_core::sc_object*); }