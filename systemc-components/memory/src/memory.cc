#include "memory.h"

typedef gs::memory<> memory;

void module_register()
{
    GSC_MODULE_REGISTER_C(memory);
}