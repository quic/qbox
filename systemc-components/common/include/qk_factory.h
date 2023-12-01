/*
 * Copyright (c) 2022 GreenSocs
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
