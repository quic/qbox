#include "greensocs/base-components/memory_dumper.h"

typedef gs::memory_dumper<> memory_dumper;

void module_register()
{
    GSC_MODULE_REGISTER_C(memory_dumper);
}