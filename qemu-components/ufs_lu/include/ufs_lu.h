/*
 * This file is part of libqbox
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_UFS_LU_H
#define _LIBQBOX_COMPONENTS_UFS_LU_H

#include <module_factory_registery.h>
#include "ufs.h"
#include <string>
#include <sstream>

class ufs_lu : public ufs::Device
{
protected:
    cci::cci_param<uint32_t> p_lun;
    cci::cci_param<std::string> blkdev_str;

private:
    std::string blkdev_id;

public:
    ufs_lu(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : ufs_lu(name, *(dynamic_cast<QemuInstance*>(o)), dynamic_cast<ufs*>(t))
    {
    }
    ufs_lu(const sc_core::sc_module_name& n, QemuInstance& inst, ufs* _ufs)
        : ufs::Device(n, inst, "ufs-sysbus-lu")
        , p_lun("lun", 0, "LU number")
        , blkdev_str("blkdev_str", "", "blkdev string for QEMU (do not specify ID)")
        , blkdev_id(std::string(name()) + "-id")
    {
        std::stringstream opts;
        opts << blkdev_str.get_value();
        opts << ",id=" << blkdev_id;
        m_inst.add_arg("-drive");
        m_inst.add_arg(opts.str().c_str());

        set_dev_props = [this]() -> void {
            m_dev.set_prop_uint("lun", p_lun.get_value());
            m_dev.set_prop_parse("drive", blkdev_id.c_str());
        };

        _ufs->add_device(*this);
    }
};

extern "C" void module_register();

#endif
