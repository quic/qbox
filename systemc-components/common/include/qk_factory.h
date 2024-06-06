/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QK_FACTORY_H
#define QK_FACTORY_H

#include <iostream>
#include <libgssync.h>

namespace gs {
std::shared_ptr<gs::tlm_quantumkeeper_extended> tlm_quantumkeeper_factory(std::string name);
}

#endif // QK_FACTORY_H
