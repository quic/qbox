/*
 * Copyright (C) 2020 GreenSocs
 */

#ifndef QKMULTI_UNCONSTRAINED_H
#define QKMULTI_UNCONSTRAINED_H

#include "greensocs/libgssync/qkmultithread.h"

namespace gs {
class tlm_quantumkeeper_unconstrained : public tlm_quantumkeeper_multithread {
    virtual sc_core::sc_time time_to_sync() override
    {
        if (status != RUNNING)
            return sc_core::SC_ZERO_TIME;
        return tlm_utils::tlm_quantumkeeper::get_global_quantum();
    }

    virtual bool need_sync() override
    {
        return false;
    }
};
}
#endif
