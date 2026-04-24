/*
 * This file is part of libqbox
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <device.h>
#include <qemu-instance.h>
#include <module_factory_registery.h>

class hexagon_tlb : public QemuDevice
{
public:
    cci::cci_param<uint32_t> p_num_entries;
    cci::cci_param<uint32_t> p_dma_entries;

    hexagon_tlb(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : hexagon_tlb(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }

    hexagon_tlb(const sc_core::sc_module_name& n, QemuInstance& inst)
        : QemuDevice(n, inst, "hexagon-tlb")
        , p_num_entries("num_entries", 128, "number of Joint TLB entries")
        , p_dma_entries("dma_entries", 0, "number of DMA TLB entries")
    {
    }

    void end_of_elaboration() override
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();
    }

    void before_end_of_elaboration() override
    {
        QemuDevice::before_end_of_elaboration();
        qemu::Device dev = get_qemu_dev();
        dev.set_prop_int("num-entries", p_num_entries);
        if (p_dma_entries > 0) {
            dev.set_prop_int("dma-entries", p_dma_entries);
        }
    }
};

extern "C" void module_register();
