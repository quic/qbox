/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
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

#ifndef _LIBQBOX_COMPONENTS_PCI_NVME_H
#define _LIBQBOX_COMPONENTS_PCI_NVME_H

#include <cci_configuration>

#include <greensocs/libgssync.h>

#include "gpex.h"

class QemuNvmeDisk : public QemuGPEX::Device {
// TODO: use a real backend object
protected:
    cci::cci_param<std::string> p_serial;
    cci::cci_param<std::string> p_blob_file;
    std::string m_drive_id;

public:
    QemuNvmeDisk(const sc_core::sc_module_name &name, QemuInstance &inst)
        : QemuGPEX::Device(name, inst, "nvme")
        , p_serial("serial", basename(), "Serial name of the nvme disk")
        , p_blob_file("blob-file", "", "Blob file to load as data storage")
        , m_drive_id(basename())
    {
        m_drive_id += "_drive";
        std::string file = p_blob_file;

        std::stringstream opts;
        opts << "if=sd,id=" << m_drive_id
             << ",file=" << file
             << ",format=raw";
        m_inst.add_arg("-drive");
        m_inst.add_arg(opts.str().c_str());
    }

    void before_end_of_elaboration() override
    {
        QemuGPEX::Device::before_end_of_elaboration();

        std::string serial = p_serial;
        m_dev.set_prop_str("serial", serial.c_str());
        m_dev.set_prop_parse("drive", m_drive_id.c_str());
    }

    void gpex_realize(qemu::Bus &bus) override
    {
        QemuGPEX::Device::gpex_realize(bus);
    }
};
#endif
