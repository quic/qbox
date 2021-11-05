/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 *  Author: Alwalid Salama
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

#include <string>
#include <functional>

#include <libqemu-cxx/target/riscv.h>

#include <greensocs/libgssync.h>

#include "libqbox/components/cpu/cpu.h"

class QemuCpuSifiveX280: public QemuCpu {

protected:
    uint64_t m_hartid;
    gs::async_event m_irq_ev;

    void mip_update_cb(uint32_t value)
    {
        if (value) {
            m_irq_ev.notify();
        }
    }

public:
    QemuCpuSifiveX280(const sc_core::sc_module_name &name,
                   QemuInstance &inst, const char *model,
                   uint64_t hartid)
        : QemuCpu(name, inst, std::string(model) + "-riscv")
        , m_hartid(hartid)
          /*
           * We have no choice but to attach-suspend here. This is fixable but
           * non-trivial. It means that the SystemC kernel will never starve...
           */
        , m_irq_ev(true)
    {
        m_external_ev |= m_irq_ev;
    }


    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuRiscv64SiFiveX280 cpu(get_qemu_dev());
        cpu.set_prop_int("hartid", m_hartid);

        cpu.set_mip_update_callback(std::bind(&QemuCpuSifiveX280::mip_update_cb,
                                        this, std::placeholders::_1));
    }


// properties to be set from main.cc
};
