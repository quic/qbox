#include "reg_router.h"

typedef gs::reg_router<> reg_router;

void module_register() { GSC_MODULE_REGISTER_C(reg_router); }