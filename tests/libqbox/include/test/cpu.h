/*
 * This file is part of libqbox
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TESTS_INCLUDE_TEST_CPU_H
#define TESTS_INCLUDE_TEST_CPU_H

#include <keystone/keystone.h>

#include <systemc>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <tlm_utils/simple_target_socket.h>

#include <cci_configuration>
#include <cci/utils/broker.h>

#include <gs_memory.h>
#include <router.h>

#include <ports/initiator-signal-socket.h>

#include <scp/report.h>

#include "qemu-instance.h"
#include "test/test.h"
#include "test/tester/tester.h"
#include <tlm_sockets_buswidth.h>

class CpuTestBenchBase : public TestBench, public CpuTesterCallbackIface
{
public:
    static constexpr uint64_t MEM_ADDR = 0x0;
    static constexpr size_t MEM_SIZE = 256 * 1024;

    static constexpr uint64_t BULKMEM_ADDR = 0x100000000;
    static constexpr size_t BULKMEM_SIZE = 1024 * 1024 * 1024;

private:
    ks_arch qemu_to_ks_arch(qemu::Target arch)
    {
        switch (arch) {
        case qemu::Target::AARCH64:
            return KS_ARCH_ARM64;

        case qemu::Target::HEXAGON:
            return KS_ARCH_HEXAGON;

        default:
            SCP_FATAL(SCMOD) << "Unsupported QEMU architecture for Keystone";
            return KS_ARCH_MAX; /* avoid compiler warning */
        }
    }

protected:
    qemu::Target m_arch;

    cci::cci_param<int> p_num_cpu;
    cci::cci_param<int> p_quantum_ns;

    gs::router<> m_router;
    gs::gs_memory<> m_mem;
    gs::gs_memory<> m_bulkmem;

    void set_firmware(const char* assembly, uint64_t addr = 0)
    {
        ks_engine* ks;
        ks_err err;
        size_t size, count;
        uint8_t* fw;

        err = ks_open(qemu_to_ks_arch(m_arch), KS_MODE_LITTLE_ENDIAN, &ks);

        if (err != KS_ERR_OK) {
            SCP_FATAL(SCMOD) << "Unable to initialize keystone";
        }

        if (ks_asm(ks, assembly, addr, &fw, &size, &count) != KS_ERR_OK || size == 0) {
            std::cerr << assembly << "\n";
            std::cerr << "errno: " << ks_errno(ks) << "\n";
            std::cerr << "error: " << ks_strerror(ks_errno(ks)) << "\n";
            SCP_INFO() << assembly;
            TEST_FAIL("Unable to assemble the test firmware\n");
        }

        m_mem.load.ptr_load(fw, addr, size);

        ks_free(fw);
        ks_close(ks);
    }

public:
    CpuTestBenchBase(const sc_core::sc_module_name& n, qemu::Target arch)
        : TestBench(n)
        , m_arch(arch)
        , p_num_cpu("num_cpu", 1, "Number of CPUs to instantiate in the test")
        , p_quantum_ns("quantum_ns", 1000000, "Value of the global TLM-2.0 quantum in ns")
        , m_router("router")
        , m_mem("mem", MEM_SIZE)
        , m_bulkmem("bulkmem", BULKMEM_SIZE)
    {
        using tlm_utils::tlm_quantumkeeper;

        m_router.add_target(m_mem.socket, MEM_ADDR, MEM_SIZE);
        m_router.add_target(m_bulkmem.socket, BULKMEM_ADDR, BULKMEM_SIZE);

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);
    }

    void map_target(tlm::tlm_target_socket<DEFAULT_TLM_BUSWIDTH>& s, uint64_t addr, uint64_t size)
    {
        m_router.add_target(s, addr, size);
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len) { TEST_FAIL("Unexpected CPU read"); }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len) { TEST_FAIL("Unexpected CPU write"); }

    virtual bool dmi_request(int id, uint64_t addr, size_t len, tlm::tlm_dmi& ret)
    {
        TEST_FAIL("Unexpected DMI request");
        return false;
    }
};

template <class CPU, class TESTER>
class CpuTestBench : public CpuTestBenchBase
{
protected:
    QemuInstanceManager m_inst_manager;
    QemuInstance m_inst_a;
    QemuInstance m_inst_b;

    bool ab = false;
    sc_core::sc_vector<CPU> m_cpus;
    TESTER m_tester;

public:
    CpuTestBench(const sc_core::sc_module_name& n)
        : CpuTestBenchBase(n, CPU::ARCH)
        , m_inst_a("inst_a", &m_inst_manager, CPU::ARCH)
        , m_inst_b("inst_b", &m_inst_manager, CPU::ARCH)
        , m_cpus("cpu", p_num_cpu,
                 [this](const char* n, int i) {
                     ab = !ab;
                     return new CPU(n, ab ? m_inst_a : m_inst_b);
                 })
        , m_tester("tester", *this)
    {
        int i = 0;
        for (CPU& cpu : m_cpus) {
            m_router.add_initiator(cpu.socket);
        }
    }

    virtual void map_irqs_to_cpus(sc_core::sc_vector<InitiatorSignalSocket<bool>>& irqs) { abort(); }

    void map_halt_to_cpus(sc_core::sc_vector<sc_core::sc_out<bool>>& halt)
    {
        int i = 0;

        for (auto& cpu : m_cpus) {
            halt[i++].bind(cpu.halt);
        }
    }

    void map_reset_to_cpus(sc_core::sc_vector<sc_core::sc_out<bool>>& reset)
    {
        int i = 0;

        for (auto& cpu : m_cpus) {
            reset[i++].bind(cpu.reset);
        }
    }
};

template <class CPU, class TESTER>
class CpuArmTestBench : public CpuTestBench<CPU, TESTER>
{
public:
    CpuArmTestBench(const sc_core::sc_module_name& n): CpuTestBench<CPU, TESTER>(n)
    {
        int i = 0;
        for (CPU& cpu : CpuTestBench<CPU, TESTER>::m_cpus) {
            cpu.p_mp_affinity = i++;
            cpu.p_has_el3 = false;
        }
    }

    virtual void map_irqs_to_cpus(sc_core::sc_vector<InitiatorSignalSocket<bool>>& irqs) override
    {
        int i = 0;
        for (CPU& cpu : CpuTestBench<CPU, TESTER>::m_cpus) {
            irqs[i++].bind(cpu.irq_in);
        }
    }
};
#endif
