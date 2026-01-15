/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_PCI_NVME_H
#define _LIBQBOX_COMPONENTS_PCI_NVME_H

#include <cci_configuration>

#include <module_factory_registery.h>
#include <libgssync.h>

#include <qemu_gpex.h>

class nvme : public qemu_gpex::Device
{
    // TODO: use a real backend object
protected:
    cci::cci_param<std::string> p_serial;
    cci::cci_param<std::string> p_blob_file;
    cci::cci_param<uint32_t> max_ioqpairs;
    std::string m_drive_id;

public:
    nvme(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : nvme(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
    {
    }
    nvme(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex)
        : qemu_gpex::Device(name, inst, "nvme")
        , p_serial("serial", basename(), "Serial name of the nvme disk")
        , p_blob_file("blob_file", "", "Blob file to load as data storage")
        , max_ioqpairs("max_ioqpairs", 64, "Passed through to QEMU max_ioqpairs")
        , m_drive_id(basename())
    {
        if (p_blob_file.get_value().empty()) {
            SCP_FATAL(()) << "the nvme device needs blob_file CCI parameter!";
        }
        m_drive_id += "_drive";
        std::string file = p_blob_file.get_value();

        std::stringstream opts;
        opts << "if=sd,id=" << m_drive_id << ",file=" << file << ",format=raw";
        m_inst.add_arg("-drive");
        m_inst.add_arg(opts.str().c_str());

        gpex->add_device(*this);
    }

    void before_end_of_elaboration() override
    {
        qemu_gpex::Device::before_end_of_elaboration();

        std::string serial = p_serial;
        m_dev.set_prop_str("serial", serial.c_str());
        m_dev.set_prop_parse("drive", m_drive_id.c_str());
        m_dev.set_prop_int("max_ioqpairs", max_ioqpairs);
    }

    void gpex_realize(qemu::Bus& bus) override { qemu_gpex::Device::gpex_realize(bus); }
};

extern "C" void module_register();

#endif
