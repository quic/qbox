#include "libqbox/components/cpu/riscv64/riscv64.h"

void module_register()
{
    GSC_MODULE_REGISTER_C(qemu_cpu_riscv64, sc_core::sc_object*, uint64_t);
}