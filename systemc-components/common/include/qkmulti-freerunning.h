/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QKMULTI_FREERUNNING_H
#define QKMULTI_FREERUNNING_H

#include <qkmultithread.h>

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
