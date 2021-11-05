/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Mirela Grujic
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

#ifndef _LIBQBOX_COMPONENTS_CACHE_CTRL_SIFIVE_PL2_H
#define _LIBQBOX_COMPONENTS_CACHE_CTRL_SIFIVE_PL2_H

#include <string>
#include <cassert>

#include <cci_configuration>

#include "libqbox/ports/target.h"
#include "libqbox/ports/target-signal-socket.h"

class QemuRiscvSifivePl2 : public QemuDevice {
public:
    QemuTargetSocket<> socket;

    QemuRiscvSifivePl2(sc_core::sc_module_name nm, QemuInstance &inst)
        : QemuDevice(nm, inst, "sifive,pL2Cache0")
        , socket("mem", inst)
    {}

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();
    }

    void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_dev());
        socket.init(qemu::SysBusDevice(m_dev), 0);
    }
};

#endif
