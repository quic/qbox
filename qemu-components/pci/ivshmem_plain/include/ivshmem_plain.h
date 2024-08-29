/*
 * This file is part of libqbox
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_RTL8139_PCI_H
#define _LIBQBOX_COMPONENTS_RTL8139_PCI_H

#include <cci_configuration>

#include <libgssync.h>
#include <qemu-instance.h>

#include <module_factory_registery.h>

#include <qemu_gpex.h>

class ivshmem_plain : public qemu_gpex::Device
{
    cci::cci_param<std::string> p_shm_path;
    cci::cci_param<uint32_t> p_shm_size;
    std::string p_shm_id;

public:
    ivshmem_plain(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : ivshmem_plain(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
    {
    }
    ivshmem_plain(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex)
        : qemu_gpex::Device(name, inst, "ivshmem-plain")
        , p_shm_path("shm_path", "", "path of shm file (i.e. /dev/shm/<name of the shm segment>)")
        , p_shm_size("shm_size", 1024, "size of the shm in MB")
        , p_shm_id(std::string(sc_core::sc_module::name()) + "-id")
    {
        std::stringstream object_string;
        object_string << "memory-backend-file,size=" << p_shm_size.get_value()
                      << "M,mem-path=" << p_shm_path.get_value() << ",share=on,id=" << p_shm_id;

        m_inst.add_arg("-object");
        m_inst.add_arg(object_string.str().c_str());

        gpex->add_device(*this);
    }

    void before_end_of_elaboration() override
    {
        qemu_gpex::Device::before_end_of_elaboration();
        m_dev.set_prop_str("memdev", p_shm_id.c_str());
    }

    ~ivshmem_plain()
    {
        // in case Qemu didn't unlink the file at exit
        unlink(p_shm_path.get_value().c_str());
    }
};

extern "C" void module_register();

#endif
