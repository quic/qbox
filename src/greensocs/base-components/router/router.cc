#include "greensocs/base-components/router.h"

typedef gs::router<> router;

void module_register()
{
    GSC_MODULE_REGISTER_C(router);
}