/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps and Luc Michel
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

#include "libqbox/libqbox.h"

class CpuArmCortexA53 : public QemuCpu {
public:
	QemuArmGic gic;

	CpuArmCortexA53(sc_core::sc_module_name name)
		: QemuCpu(name, "cortex-a53-arm")
        , gic("gic")
	{
    }

    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuAarch64 cpu = qemu::CpuAarch64(get_qemu_obj());
        cpu.set_aarch64_mode(true);
        cpu.set_prop_bool("has_el2", true);
        cpu.set_prop_bool("has_el3", false);

        /* create the gic in the same QEMU instance */
        gic.set_qemu_instance(*m_lib);

        gic.m_cpu = this;
    }
};
