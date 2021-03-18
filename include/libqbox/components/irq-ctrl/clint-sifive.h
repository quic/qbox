/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Lukas Jünger
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

#include <vector>

#include <cci_configuration>

#include <libqemu-cxx/target/riscv.h>

#include "libqbox/ports/target.h"

class QemuRiscvSifiveClint : public QemuDevice {
protected:
    uint64_t m_aperture_size;
    int m_num_harts;

public:
    cci::cci_param<unsigned int> p_num_harts;
    cci::cci_param<uint64_t> p_sip_base;
    cci::cci_param<uint64_t> p_timecmp_base;
    cci::cci_param<uint64_t> p_time_base;
    cci::cci_param<bool> p_provide_rdtime;
    cci::cci_param<uint64_t> p_aperture_size;

    QemuTargetSocket<> socket;

    QemuRiscvSifiveClint(sc_core::sc_module_name nm, QemuInstance &inst)
            : QemuDevice(nm, inst, "riscv.sifive.clint")
            , p_num_harts("num-harts", 0, "Number of HARTS this CLINT is connected to")
            , p_sip_base("sip-base", 0, "Base address for the SIP registers")
            , p_timecmp_base("timecmp-base", 0, "Base address for the TIMECMP registers")
            , p_time_base("time-base", 0, "Base address for the TIME registers")
            , p_provide_rdtime("provide-rdtime", false, "If true, provide the CPU with "
                                                        "a rdtime register")
            , p_aperture_size("aperture-size", 0, "Size of the whole CLINT address space")
            , socket("mem", inst)
    {}

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_int("aperture-size", p_aperture_size);
        m_dev.set_prop_int("num-harts", p_num_harts);
        m_dev.set_prop_int("sip-base", p_sip_base);
        m_dev.set_prop_int("timecmp-base", p_timecmp_base);
        m_dev.set_prop_int("time-base", p_time_base);
        m_dev.set_prop_bool("provide-rdtime", p_provide_rdtime);
    }

    void end_of_elaboration() override
    {
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(get_qemu_dev());
        socket.init(qemu::SysBusDevice(m_dev), 0);
    }
};
