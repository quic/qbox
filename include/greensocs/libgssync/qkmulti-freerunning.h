/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

#ifndef QKMULTI_FREERUNNING_H
#define QKMULTI_FREERUNNING_H

#include "greensocs/libgssync/qkmultithread.h"

namespace gs {
class tlm_quantumkeeper_freerunning : public tlm_quantumkeeper_multithread
{
    virtual void sync() override
    {
        if (is_sysc_thread()) {
            assert(m_local_time >= sc_core::sc_time_stamp());
            sc_core::sc_time t = m_local_time - sc_core::sc_time_stamp();
            m_tick.notify();
            sc_core::wait(t);
        } else {
            /* Wake up the SystemC thread if it's waiting for us to keep up */
            m_tick.notify();
        }
    }

    virtual bool need_sync() override { return false; }
};
} // namespace gs
#endif
