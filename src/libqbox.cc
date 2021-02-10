#include "libqbox/libqbox.h"

bool QemuCpu::dmi_force_exit_on_io = false;

std::vector<struct QemuCpu::dmi_region> QemuCpu::g_dmis;
