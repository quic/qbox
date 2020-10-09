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

#include <libqemu-cxx/target/arm.h>

#include "nvic.h"

class CpuArmCortexM7 : public QemuCpu {
public:
	QemuArmNvic nvic;

	CpuArmCortexM7(sc_core::sc_module_name name, uint32_t nvic_num_irq = 64)
		: QemuCpu(name, "aarch64", "cortex-m7-arm")
		, nvic("nvic", this, nvic_num_irq)
	{
    }

    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuArm cpu = qemu::CpuArm(m_obj);
        cpu.add_nvic_link();
        get_qemu_obj().set_prop_link("nvic", nvic.get_qemu_obj());
    }
};
