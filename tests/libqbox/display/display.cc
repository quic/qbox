/*
 *  This file is part of libqbox
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

#include <iostream>

#include <greensocs/base-components/router.h>
#include <greensocs/base-components/memory.h>
#include <libqbox-extra/components/display/display.h>
#include <libqbox/components/gpu/virtio-mmio-gpugl.h>

#include "test/test.h"

class DisplayTest : public TestBench
{
private:
    QemuInstanceManager m_inst_manager;
    QemuInstance* m_inst;
    std::unique_ptr<QemuVirtioMMIOGpuGl> m_gpu;
    std::unique_ptr<QemuDisplay> m_display;

public:
    DisplayTest(const sc_core::sc_module_name& n): TestBench(n) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            // Skip this test on platforms with no video device available
            SCP_WARN(SCMOD) << "Skipping Display test: Failed to initialize SDL: "
                            << SDL_GetError();
            return;
        }

        m_inst = &m_inst_manager.new_instance("inst", qemu::Target::AARCH64);
        m_gpu = std::make_unique<QemuVirtioMMIOGpuGl>("gpu", *m_inst);
        m_display = std::make_unique<QemuDisplay>("display", *m_gpu);
    }

    virtual void end_of_simulation() override {
        TestBench::end_of_simulation();

        if (m_display) {
            TEST_ASSERT(m_display->is_instantiated());
            TEST_ASSERT(m_display->is_realized());
            TEST_ASSERT(!m_display->get_sdl2_consoles().empty());
        }
    }
};

int sc_main(int argc, char* argv[]) {
    return run_testbench<DisplayTest>(argc, argv);
}
