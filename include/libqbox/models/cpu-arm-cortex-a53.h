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

#pragma once

#include "libqbox/libqbox.h"

#include <libqemu-cxx/target/arm.h>

#include "gic.h"

class CpuArmCortexA53 : public QemuCpu {
public:
	CpuArmCortexA53(sc_core::sc_module_name name)
		: QemuCpu(name, "cortex-a53-arm")
	{
        m_max_access_size = 8;
    }

    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuAarch64 cpu = qemu::CpuAarch64(get_qemu_obj());
        cpu.set_aarch64_mode(true);
        cpu.set_prop_bool("has_el2", true);
        cpu.set_prop_bool("has_el3", false);
    }

    void end_of_elaboration()
    {
        QemuCpu::end_of_elaboration();
    }
};

class CpuArmCortexA53Cluster : sc_core::sc_module {
public:
    sc_core::sc_vector<CpuArmCortexA53> cores;

    QemuArmGic gic;

    CpuArmCortexA53Cluster(sc_core::sc_module_name name, uint8_t ncores)
		: sc_module(name)
        , cores("core", ncores, [this](const char *cname, size_t i) { return new CpuArmCortexA53(cname); })
        , gic("gic")
	{
        /* 1-4 cores per cluster */
        assert(ncores <= 4);

        for (int i = 0; i < ncores; i++) {
            if (i > 0) {
                cores[0].add_to_qemu_instance(&cores[i]);
            }
            gic.add_core(&cores[i]);
        }
    }
};
