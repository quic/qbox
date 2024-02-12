#include "gs_memory.h"

typedef gs::gs_memory<> gs_memory;

void module_register() { GSC_MODULE_REGISTER_C(gs_memory); }