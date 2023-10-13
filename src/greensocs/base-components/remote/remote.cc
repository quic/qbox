#include "greensocs/base-components/remote.h"

typedef gs::LocalPass<> LocalPass;
typedef gs::RemotePass<> RemotePass;

void module_register()
{
    GSC_MODULE_REGISTER_C(LocalPass);
    GSC_MODULE_REGISTER_C(RemotePass);
}