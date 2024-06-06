#include <loader.h>

typedef gs::loader<> loader;

void module_register() { GSC_MODULE_REGISTER_C(loader); }