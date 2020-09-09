/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps
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

#pragma once

#include <systemc>
#include "tlm.h"

#include <libqemu-cxx/target/arm.h>

#include "gic.h"

class QemuLan9118 : public QemuComponent {
public:
    QemuSignal irq;

    tlm_utils::simple_target_socket<QemuLan9118> socket;

protected:
    using QemuComponent::m_obj;

public:
    QemuLan9118(sc_core::sc_module_name name)
        : QemuComponent(name, "lan9118")
        , irq("irq")
    {
    }

    ~QemuLan9118()
    {
    }

    void set_backend_slirp()
    {
        m_extra_qemu_args.push_back("-netdev");
        m_extra_qemu_args.push_back("user,id=net0");
    }

    void set_backend_tap(std::string name = "qbox0")
    {
        std::stringstream ss;
        m_extra_qemu_args.push_back("-netdev");
        ss << "tap,id=net0,ifname=" << name << ",script=no,downscript=no";
        m_extra_qemu_args.push_back(ss.str());
    }

    void set_qemu_properties_callback()
    {
        m_obj.set_prop_str("netdev", "net0");
    }

    void set_qemu_instance_callback()
    {
        QemuComponent::set_qemu_instance_callback();

        set_qemu_properties_callback();
    }

    void before_end_of_elaboration()
    {
    }

    void end_of_elaboration()
    {
        QemuComponent::end_of_elaboration();

        qemu::SysBusDevice sbd = qemu::SysBusDevice(get_qemu_obj());

        if(QemuInPort *port = dynamic_cast<QemuInPort *>(irq.bound_port)) {
            sbd.connect_gpio_out(0, port->get_gpio());
        }
        else {
            printf("error: connecting irq outside qemu is not yet implemented\n");
            exit(1);
        }
    }
};
