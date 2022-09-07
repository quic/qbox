/*
 */

#include "quic/smmu500/smmu500.h"
#include <greensocs/gsutils/cciutils.h>


int sc_main(int argc, char* argv[]) {
    auto m_broker = new gs::ConfigurableBroker(argc, argv);
    smmu500<> smmu("test_smmu");
    return 0;
}
