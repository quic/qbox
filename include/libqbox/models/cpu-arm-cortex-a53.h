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
    int affinity;

	CpuArmCortexA53(sc_core::sc_module_name name)
		: QemuCpu(name, "cortex-a53-arm")
	{
        m_max_access_size = 8;
        m_affinity = -1;
    }

    void before_end_of_elaboration()
    {
        QemuCpu::before_end_of_elaboration();

        qemu::CpuAarch64 cpu = qemu::CpuAarch64(get_qemu_obj());
        cpu.set_aarch64_mode(true);
        if (m_affinity >= 0) {
            cpu.set_prop_int("mp-affinity", affinity);
        }
        cpu.set_prop_bool("has_el2", true);
        cpu.set_prop_bool("has_el3", false);
    }

    void end_of_elaboration()
    {
        QemuCpu::end_of_elaboration();
    }

    virtual bool before_b_transport(tlm::tlm_generic_payload &trans,
            qemu::MemoryRegionOps::MemTxAttrs &attrs)
    {
        if (attrs.exclusive) {
            if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
                /*
                 * cmpxchg write:
                 * QEMU will do a write transaction in exclusive store,
                 * regardless the result of the compare.
                 * In such a case, skip the write and simply return OK
                 * to prevent QEMU from generating a CPU fault.
                 * The exclusive store will fail anyway, thanks to the
                 * instrumentation in read path below.
                 */
                trans.set_response_status(tlm::TLM_OK_RESPONSE);
                return false;
            }
        }
        return true;
    }

    virtual void after_b_transport(tlm::tlm_generic_payload &trans,
            qemu::MemoryRegionOps::MemTxAttrs &attrs)
    {
        if (attrs.exclusive) {
            if (trans.get_command() == tlm::TLM_READ_COMMAND) {
                /*
                 * cmpxchg read:
                 * make QEMU fail the compare and exchange in exclusive store
                 * by returning an unexpected value.
                 */
                qemu::CpuArm cpu = qemu::CpuArm(get_qemu_obj());
                uint64_t e_val = cpu.get_exclusive_val();
                uint8_t *ptr = trans.get_data_ptr();
                *(uint64_t *)ptr = ~e_val;
                trans.set_response_status(tlm::TLM_OK_RESPONSE);
            }
        }
    }
};
