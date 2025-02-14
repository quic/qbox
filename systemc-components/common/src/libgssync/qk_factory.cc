/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <qk_factory.h>

namespace gs {
std::shared_ptr<gs::tlm_quantumkeeper_extended> tlm_quantumkeeper_factory(std::string name)
{
    if (name == "tlm2") return std::make_shared<gs::tlm_quantumkeeper_extended>();
    if (name == "multithread") return std::make_shared<gs::tlm_quantumkeeper_multithread>();
    if (name == "multithread-quantum") return std::make_shared<gs::tlm_quantumkeeper_multi_quantum>();
    if (name == "multithread-adaptive") return std::make_shared<gs::tlm_quantumkeeper_multi_adaptive>();
    if (name == "multithread-rolling") return std::make_shared<gs::tlm_quantumkeeper_multi_rolling>();
    if (name == "multithread-unconstrained") return std::make_shared<gs::tlm_quantumkeeper_unconstrained>();
    if (name == "multithread-freerunning") return std::make_shared<gs::tlm_quantumkeeper_freerunning>();
    return nullptr;
}
} // namespace gs
