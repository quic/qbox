/*
 * This file is part of libgsutils
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBGSUTILS_PORTS_INITIATOR_SIGNAL_SOCKET_H
#define _LIBGSUTILS_PORTS_INITIATOR_SIGNAL_SOCKET_H

#include <systemc>

template <class T>
using InitiatorSignalSocket = sc_core::sc_port<sc_core::sc_signal_inout_if<T>, 1, sc_core::SC_ZERO_OR_MORE_BOUND>;

template <typename T = bool>
class MultiInitiatorSignalSocket
    : public sc_core::sc_port<sc_core::sc_signal_inout_if<T>, 0, sc_core::SC_ZERO_OR_MORE_BOUND>,
      public sc_core::sc_module
{
    std::vector<T> vals;
    sc_core::sc_event ev;

public:
    MultiInitiatorSignalSocket(const char* name = "MultiInitiatorSignalSocket")
        : sc_core::sc_port<sc_core::sc_signal_inout_if<T>, 0, sc_core::SC_ZERO_OR_MORE_BOUND>(name)
        , sc_core::sc_module(sc_core::sc_module_name((std::string(name) + "_thread").c_str()))
    {
        SC_THREAD(writer);
    }
    void writer()
    {
        while (1) {
            wait(ev);
            std::vector<T> vs = vals; // No race conditions, we are single threaded. COPY the vector
            for (auto v : vs) {
                for (int i = 0; i < this->size(); i++) {
                    this->operator[](i)->write(v);
                }
            }
        }
    }
    void async_write_vector(const std::vector<T>& vs)
    {
        vals = vs; // COPY the vector.
        ev.notify();
    }
};

#endif
