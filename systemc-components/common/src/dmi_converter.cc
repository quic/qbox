#include <dmi_converter.h>

typedef gs::dmi_converter<> dmi_converter;

void module_register() { GSC_MODULE_REGISTER(dmi_converter); }
