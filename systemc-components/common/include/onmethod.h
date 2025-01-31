/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ONMETHODHELPER_H
#define ONMETHODHELPER_H

#include <systemc>

class onMethodHelper : public sc_core::sc_module
{
    std::function<void()> entry;
    sc_core::sc_event start;
    sc_core::sc_event done;
    void start_entry()
    {
        entry();
        done.notify();
    }

public:
    onMethodHelper(): sc_core::sc_module(sc_core::sc_module_name("onMethodHelper"))
    {
        SC_METHOD(start_entry);
        sensitive << start;
        dont_initialize();
    }
    void run(std::function<void()> fn)
    {
        entry = fn;
        auto kind = sc_core::sc_get_current_process_handle().proc_kind();
        if (kind == sc_core::SC_THREAD_PROC_) {
            start.notify();
            wait(done);
        } else {
            entry();
        }
    }
};

#endif // ONMETHODHELPER_H
