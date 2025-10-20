// Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include "router.h"
#include "tlm_utils/tlm_quantumkeeper.h"
#include "tlm_utils/simple_target_socket.h"

#include "test.h"
#include "hexagon.h"
#include "mmio_probe.h"
#include "hexagon_globalreg.h"

class LoadStoreTest : public sc_core::sc_module, public MmioWriter, public MmioReader
{
private:
    static constexpr const char* FIRMWARE = R"(
        _start:
            r0 = #0x%08)" PRIx32 R"(

            //load and discard the result from CpuTesterMmio::MMIO_ADDR:
            r2 = memw(r0)

            //store a known value to CpuTesterMmio::MMIO_ADDR:
            r3 = #0x0f0f0f0f
            memw(r0) = r3
        end:
            // This instruction is not supported by keystone:
            //   wait(r0)
            // So we'll just bypass it and use the word we got from a reference
            //   assembler:
            .word 0x6440c000
            jump end
    )";

    static constexpr uint64_t MEM_ADDR = 0x0;
    static constexpr size_t MEM_SIZE = 256 * 1024;
    static constexpr uint32_t QUANTUM = 1000000;

    QemuInstanceManager qemu_instance_manager;
    QemuInstance qemu_instance;

    qemu_cpu_hexagon cpu;
    hexagon_globalreg hex_gregs;
    gs::gs_memory<> memory;
    gs::router<> router;

    MmioProbe mmio_probe;

    bool test_passed;

public:
    LoadStoreTest(const sc_core::sc_module_name& module_name)
        : sc_core::sc_module(module_name)
        , qemu_instance("qemu_instance", &qemu_instance_manager, qemu_cpu_hexagon::ARCH)
        , cpu("hexagon_cpu", qemu_instance)
        , memory("memory")
        , router("router")
        , mmio_probe("mmio_probe", router)
        , test_passed(false)
        , hex_gregs("hexagon_globalreg", &qemu_instance)
    {
        sc_core::sc_time global_quantum(QUANTUM, sc_core::SC_NS);
        tlm_utils::tlm_quantumkeeper::set_global_quantum(global_quantum);

        hex_gregs.p_hexagon_start_addr = MEM_ADDR;

        router.add_target(memory.socket, MEM_ADDR, MEM_SIZE);
        router.add_initiator(cpu.socket);

        char buf[1024];
        std::snprintf(buf, sizeof(buf), FIRMWARE, static_cast<uint32_t>(MmioProbe::MMIO_ADDR));
        load_firmware(memory, buf, MEM_ADDR);

        mmio_probe.connect_to_mmio_writer(this);
        mmio_probe.connect_to_mmio_reader(this);
    }

    virtual ~LoadStoreTest() {}

    void before_end_of_elaboration() override
    {
        sc_core::sc_module::before_end_of_elaboration();
        hex_gregs.before_end_of_elaboration();
        qemu::Device hex_gregs_dev = hex_gregs.get_qemu_dev();
        cpu.before_end_of_elaboration();
        qemu::Device cpu_dev = cpu.get_qemu_dev();
        cpu_dev.set_prop_link("global-regs", hex_gregs_dev);
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) override
    {
        SCP_INFO() << "write, data: 0x" << std::hex << data << ", len: 0x" << len;
        test_passed = (addr == 0 && data == 0x0f0f0f0f && len == sizeof(int32_t));
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) override { return 0; }

    virtual void end_of_simulation() override { TEST_ASSERT(test_passed); }
};

RUN_TEST(LoadStoreTest)
