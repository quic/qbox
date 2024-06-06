#include <pass.h>

typedef gs::pass<> pass;

void module_register() { GSC_MODULE_REGISTER_C(pass); }
