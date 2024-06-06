#include <virtio-keyboard-pci.h>

void module_register() { GSC_MODULE_REGISTER_C(virtio_keyboard_pci, sc_core::sc_object*, sc_core::sc_object*); }