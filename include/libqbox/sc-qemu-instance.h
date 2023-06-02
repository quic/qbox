/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm
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

// #ifndef LIBQBOX_SC_QEMU_INSTANCE_H_
// #define LIBQBOX_SC_QEMU_INSTANCE_H_

#include <cassert>
#include <sstream>
#include <systemc>

#include <cci_configuration>
#include <vector>

#include "qemu-instance.h"

#ifndef SC_QEMU_INSTANCE_H
#define SC_QEMU_INSTANCE_H

SC_MODULE(SC_QemuInstanceManager)
{

public:
    QemuInstanceManager m_inst_mgr;

    QemuInstanceManager& getInstMng(){
        return m_inst_mgr;
    }

    SC_QemuInstanceManager(const sc_core::sc_module_name& n)
    {
    }
};

/* should be in Qbox */
SC_MODULE(SC_QemuInstance)
{
private:
    QemuInstance& qemu_inst;
    QemuInstance& strtotarget(SC_QemuInstanceManager* inst_mgr, std::string s)
    {
        if (s == "AARCH64") {
            SCP_INFO(SCMOD) << "Making an AARCH64 target instance\n";
            return inst_mgr->getInstMng().new_instance(QemuInstance::Target::AARCH64);
        }
        else if (s == "RISCV64") {
            SCP_INFO(SCMOD) << "Making an RISCV64 target instance\n";
            return inst_mgr->getInstMng().new_instance(QemuInstance::Target::RISCV64);
        }
        else if (s == "HEXAGON") {
            SCP_INFO(SCMOD) << "Making an RISCV64 target instance\n";
            return inst_mgr->getInstMng().new_instance(QemuInstance::Target::HEXAGON);
        }
        else {
            SCP_FATAL(SCMOD) << "Unable to find QEMU target container";
        }
    }

public:
    QemuInstance& getQemuInst(){
        return qemu_inst;
    }

    SC_QemuInstance(const sc_core::sc_module_name& n, sc_core::sc_object* o, std::string arch)
        : qemu_inst(strtotarget(dynamic_cast<SC_QemuInstanceManager*>(o), arch))
    {
    }
};

#endif // SC_QEMU_INSTANCE_H