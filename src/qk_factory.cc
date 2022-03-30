/*
* Copyright (c) 2022 GreenSocs
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version, or under the
* Apache License, Version 2.0 (the "License”) at your discretion.
*
* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
* You may obtain a copy of the Apache License at
* http://www.apache.org/licenses/LICENSE-2.0
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
