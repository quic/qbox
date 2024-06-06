#include <virtio-mouse-pci.h>

void module_register() { GSC_MODULE_REGISTER_C(virtio_mouse_pci, sc_core::sc_object*, sc_core::sc_object*); }