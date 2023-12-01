#include <tlm_bus_width_bridges.h>

typedef gs::tlm_bus_width_bridges<32, 64> TLMBUSWIDTHBridgeFrom32To64;
typedef gs::tlm_bus_width_bridges<64, 32> TLMBUSWIDTHBridgeFrom64To32;

void module_register()
{
    GSC_MODULE_REGISTER_C(TLMBUSWIDTHBridgeFrom32To64);
    GSC_MODULE_REGISTER_C(TLMBUSWIDTHBridgeFrom64To32);
}
