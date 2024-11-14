/*
 * This file is part of libqbox
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_SIFIVE_TEST_H
#define _LIBQBOX_COMPONENTS_SIFIVE_TEST_H

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <systemc>
#include <cci_configuration>
#include <module_factory_registery.h>
#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_sockets_buswidth.h>

class sifive_test : public QemuDevice
{
private:
    sc_core::sc_time last_reset;
    sc_core::sc_event reset_ev;

public:
    QemuTargetSocket<> q_socket;
    tlm_utils::simple_initiator_socket<sifive_test, DEFAULT_TLM_BUSWIDTH> initiator_socket;
    tlm_utils::simple_target_socket<sifive_test, DEFAULT_TLM_BUSWIDTH> target_socket;
    sc_core::sc_port<sc_core::sc_signal_inout_if<bool>, 0, sc_core::SC_ZERO_OR_MORE_BOUND> reset;

    sifive_test(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : sifive_test(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    sifive_test(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "riscv.sifive.test")
        , last_reset(sc_core::SC_ZERO_TIME)
        , q_socket("mem", inst)
        , initiator_socket("initiator_socket")
        , target_socket("target_socket")
        , reset("reset")
    {
        initiator_socket.bind(q_socket);
        target_socket.register_b_transport(this, &sifive_test::b_transport);
        SC_METHOD(do_reset);
        sensitive << reset_ev;
        dont_initialize();
    }

    void before_end_of_elaboration() override { QemuDevice::before_end_of_elaboration(); }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();
        qemu::SysBusDevice sbd(m_dev);
        q_socket.init(sbd, 0);
    }

    void do_reset()
    {
        if (last_reset == sc_core::sc_time_stamp()) return;
        SCP_INFO(())("Initiate Reset sequence");
        for (int i = 0; i < reset.size(); i++) {
            reset[i]->write(1);
        }
        for (int i = 0; i < reset.size(); i++) {
            reset[i]->write(0);
        }
        last_reset = sc_core::sc_time_stamp();
    }

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        initiator_socket->b_transport(trans, delay);
        sc_core::wait(sc_core::SC_ZERO_TIME);
        if (trans.get_address() == 0x0) {
            reset_ev.notify(sc_core::SC_ZERO_TIME);
        }
    }
};

extern "C" void module_register();

#endif
