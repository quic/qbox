/*
 * Copyright (C) 2020 GreenSocs
 */

#include "greensocs/libgssync/qk_factory.h"

namespace gs {
    std::shared_ptr<gs::tlm_quantumkeeper_extended> tlm_quantumkeeper_factory(std::string name) {
        if (name == "tlm2")
            return std::make_shared<gs::tlm_quantumkeeper_extended>();
        if (name == "multithread")
            return std::make_shared<gs::tlm_quantumkeeper_multithread>();
        if (name == "multithread-quantum")
            return std::make_shared<gs::tlm_quantumkeeper_multi_quantum>();
        if (name == "multithread-rolling")
            return std::make_shared<gs::tlm_quantumkeeper_multi_rolling>();
        if (name == "multithread-unconstrained")
            return std::make_shared<gs::tlm_quantumkeeper_unconstrained>();
        return nullptr;
    }
}
