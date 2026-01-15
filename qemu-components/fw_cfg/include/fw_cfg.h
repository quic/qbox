/*
 * This file is part of libqbox
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_FW_CFG_H
#define _LIBQBOX_COMPONENTS_FW_CFG_H

#include <cci_configuration>
#include <module_factory_registery.h>
#include <qemu-instance.h>
#include <device.h>
#include <ports/target.h>
#include <limits>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>

class fw_cfg : public QemuDevice
{
public:
    QemuTargetSocket<> ctl_target_socket;
    QemuTargetSocket<> data_target_socket;
    QemuTargetSocket<> dma_target_socket;

private:
    QemuInstance& m_inst;
    cci::cci_param<uint32_t> p_data_width;
    cci::cci_param<uint32_t> p_num_cpus;
    cci::cci_param<std::string> p_acpi_tables;
    std::vector<uint8_t> acpi_tables_data;
    cci::cci_param<std::string> p_acpi_tables_linker;
    std::vector<uint8_t> acpi_tables_linker_data;
    cci::cci_param<std::string> p_acpi_rsdp;
    std::vector<uint8_t> acpi_rsdp_data;
    const uint16_t fw_cfg_num_cpus = 0x05;
    FWCfgState* fw_cfg_ptr;

public:
    fw_cfg(const sc_core::sc_module_name& name, sc_core::sc_object* o): fw_cfg(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    fw_cfg(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "fw_cfg_mem")
        , m_inst(inst)
        , ctl_target_socket("ctl_target_socket", inst)
        , data_target_socket("data_target_socket", inst)
        , dma_target_socket("dma_target_socket", inst)
        , p_data_width("data_width", std::numeric_limits<uint32_t>::max(), "data width")
        , p_num_cpus("num_cpus", 1, "number of cpus used in the platform")
        , p_acpi_tables("acpi_tables", "", "acpi tables blob binary file path")
        , p_acpi_tables_linker("acpi_tables_linker", "", "acpi tables linker blob binary file path")
        , p_acpi_rsdp("acpi_rsdp", "", "acpi rsdp blob binary file path")
    {
    }

    void file_load(const std::string& filename, std::vector<uint8_t>& file_data)
    {
        if (!std::filesystem::exists(filename)) {
            SCP_FATAL(()) << "file_load() -> error: file not found! (" << filename << ")";
        }
        auto len = std::filesystem::file_size(filename);
        if (!len) {
            SCP_FATAL(()) << "file_load() -> error: file size is 0! (" << filename << ")";
        }
        std::ifstream fin(filename, std::ios::in | std::ios::binary);
        if (!fin.good()) {
            SCP_FATAL(()) << "file_load() -> error: Can't open file for read! (" << filename << ")";
        }
        fin.seekg(0, std::ios::beg);
        file_data.resize(len);
        fin.read(reinterpret_cast<char*>(file_data.data()), len);
        fin.close();
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();
        m_dev.set_prop_bool("dma_enabled", true);
        if (!p_data_width.is_default_value()) m_dev.set_prop_uint("data_width", p_data_width.get_value());
        fw_cfg_ptr = m_inst.get().fw_cfg_find();
        if (!fw_cfg_ptr) {
            SCP_FATAL(()) << "fw_cfg device was not found!";
        }
        m_inst.get().fw_cfg_set_dma_as(fw_cfg_ptr);
    }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();
        qemu::SysBusDevice sbd(m_dev);
        ctl_target_socket.init(sbd, 0);
        data_target_socket.init(sbd, 1);
        dma_target_socket.init(sbd, 2);
        m_inst.get().fw_cfg_add_i16(fw_cfg_ptr, fw_cfg_num_cpus, p_num_cpus.get_value());
        if (!p_acpi_tables.get_value().empty()) {
            file_load(p_acpi_tables.get_value(), acpi_tables_data);
            m_inst.get().fw_cfg_modify_file(fw_cfg_ptr, "etc/acpi/tables",
                                            reinterpret_cast<void*>(acpi_tables_data.data()), acpi_tables_data.size());
        }
        if (!p_acpi_tables_linker.get_value().empty()) {
            file_load(p_acpi_tables_linker.get_value(), acpi_tables_linker_data);
            m_inst.get().fw_cfg_modify_file(fw_cfg_ptr, "etc/table-loader",
                                            reinterpret_cast<void*>(acpi_tables_linker_data.data()),
                                            acpi_tables_linker_data.size());
        }
        if (!p_acpi_rsdp.get_value().empty()) {
            file_load(p_acpi_rsdp.get_value(), acpi_rsdp_data);
            m_inst.get().fw_cfg_modify_file(fw_cfg_ptr, "etc/acpi/rsdp", reinterpret_cast<void*>(acpi_rsdp_data.data()),
                                            acpi_rsdp_data.size());
        }
    }
};

extern "C" void module_register();

#endif