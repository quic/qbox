/*
 * This file is part of libqbox
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_RESET_GPIO_INITIATOR_H
#define _LIBQBOX_COMPONENTS_RESET_GPIO_INITIATOR_H

#include <ports/qemu-initiator-signal-socket.h>
#include <ports/qemu-target-signal-socket.h>
#include <device.h>
#include <qemu-instance.h>
#include <module_factory_registery.h>
#include <ports/multiinitiator-signal-socket.h>

class reset_gpio : public QemuDevice
{
    SCP_LOGGER();
    QemuInitiatorSignalSocket m_reset_i;
    QemuTargetSignalSocket m_reset_t;

    bool m_qemu_resetting = false;
    bool m_systemc_resetting = false;
    sc_core::sc_event m_reset_ev;

public:
    MultiInitiatorSignalSocket<> reset_out;
    TargetSignalSocket<bool> reset_in;

    reset_gpio(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : reset_gpio(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    reset_gpio(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "reset_gpio")
        , m_reset_i("_qemu_reset_i")
        , m_reset_t("_qemu_reset_t")
        , reset_out("reset_out")
        , reset_in("reset_in")
    {
        SCP_TRACE(())("Init");
        m_reset_i.bind(m_reset_t);

        /* QEMU signalling reset*/
        m_reset_t.register_value_changed_cb([&](bool value) {
            sc_core::sc_suspendable(); // we should never be in a situation where the caller is waiting for us to
                                       // complete, so we can remove the unsuspendable restriction
            if (value) {
                if (!m_systemc_resetting && !m_qemu_resetting) {
                    SCP_INFO(())("QEMU resetting");
                    m_qemu_resetting = true;

                    SCP_DEBUG(())("Starting SystemC reset");
                    m_systemc_resetting = true;
                    reset_out.async_write_vector({ 1, 0 }); // Async reset systemc
                }
            } else {
                assert(m_qemu_resetting);
                // QEMU has done it's reset.
                SCP_INFO(())("QEMU done resetting");
                m_qemu_resetting = false;
                m_reset_ev.notify();
                while (m_systemc_resetting) {
                    SCP_DEBUG(())("Waiting for SystemC to be done resetting");
                    wait(m_reset_ev);
                }
                SCP_DEBUG(())("Finished waiting for SystemC reset");
            }
        });

        /* SystemC signalling reset*/
        reset_in.register_value_changed_cb([&](bool value) {
            if (value) {
                if (!m_systemc_resetting && !m_qemu_resetting) {
                    SCP_INFO(())("SystemC resetting");
                    m_systemc_resetting = true;

                    SCP_DEBUG(())("Starting QEMU reset");
                    m_qemu_resetting = true;
                    m_inst.get().system_reset(); // Async reset qemu
                }
            } else {
                assert(m_systemc_resetting);
                SCP_INFO(())("SystemC done resetting");
                m_systemc_resetting = false;
                m_reset_ev.notify();
                while (m_qemu_resetting) {
                    SCP_DEBUG(())("Waiting Qemu to be done resetting");
                    wait(m_reset_ev);
                }
                SCP_DEBUG(())("Finished waiting for QEMU reset");
            }
        });
    }

    virtual void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();
        m_reset_i.init_named(m_dev, "reset_out", 0);
    }
    virtual void start_of_simulation() override { m_dev.set_prop_bool("active", true); }
};

extern "C" void module_register();
#endif //_LIBQBOX_COMPONENTS_RESET_GPIO_INITIATOR_H
