/*
 *  This file is part of libqbox
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <iostream>

#include <display.h>
#include <virtio/virtio-mmio-gpugl.h>

#include "test/test.h"

class MainThreadDisplayTest : public TestBench
{
private:
    QemuInstanceManager m_inst_manager;
    QemuInstance m_inst;
    std::unique_ptr<QemuVirtioMMIOGpuGl> m_gpu;
    std::unique_ptr<display> m_display;

public:
    MainThreadDisplayTest(const sc_core::sc_module_name& n)
        : TestBench(n), m_inst("inst", &m_inst_manager, qemu::Target::AARCH64)
    {
        if (m_inst.get().sdl2_init() < 0) {
            // Skip this test on platforms with no video device available
            SCP_WARN(SCMOD) << "Skipping Display test: Failed to initialize SDL: " << m_inst.get().sdl2_get_error();
            return;
        }

        m_gpu = std::make_unique<QemuVirtioMMIOGpuGl>("gpu", m_inst);
        m_display = std::make_unique<display>("display", *m_gpu);
    }

    virtual void end_of_simulation() override
    {
        TestBench::end_of_simulation();

        if (m_display) {
            TEST_ASSERT(m_display->is_instantiated());
            TEST_ASSERT(m_display->is_realized());
            TEST_ASSERT(!m_display->get_sdl2_consoles()->empty());
        }
    }
};

int sc_main(int argc, char* argv[]) { return run_testbench<MainThreadDisplayTest>(argc, argv); }
