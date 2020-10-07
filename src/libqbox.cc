#include "libqbox/libqbox.h"

void copy_file(const char* srce_file, const char* dest_file)
{
    std::ifstream srce(srce_file, std::ios::binary);
    std::ofstream dest(dest_file, std::ios::binary);
    dest << srce.rdbuf();
}

bool QemuCpu::dmi_force_exit_on_io = false;

std::vector<struct QemuCpu::dmi_region> QemuCpu::g_dmis;
