/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LIBQBOX_COMPONENTS_RESET_GPIO_INITIATOR_H
#define _LIBQBOX_COMPONENTS_RESET_GPIO_INITIATOR_H

#include <ports/qemu-initiator-signal-socket.h>
#include <ports/qemu-target-signal-socket.h>
#include <device.h>
#include <qemu-instance.h>
#include <module_factory_registery.h>

class reset_gpio : public QemuDevice
{
    SCP_LOGGER();
    QemuInitiatorSignalSocket m_reset_i;
    QemuTargetSignalSocket m_reset_t;

public:
    sc_core::sc_port<sc_core::sc_signal_inout_if<bool>, 0, sc_core::SC_ZERO_OR_MORE_BOUND> reset;

    reset_gpio(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : reset_gpio(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    reset_gpio(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "reset_gpio"), m_reset_i("_qemu_reset_i"), m_reset_t("_qemu_reset_t"), reset("reset")
    {
        SCP_TRACE(())("Init");
        m_reset_i.bind(m_reset_t);
        m_reset_t.register_value_changed_cb([&](bool value) {
            for (int i = 0; i < reset.size(); i++) {
                if (auto t = dynamic_cast<TargetSignalSocketProxy<bool>*>(reset[i])) {
                    SCP_WARN(())("Reset {} to {}", value, t->get_parent().name());
                }
                reset[i]->write(value);
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
